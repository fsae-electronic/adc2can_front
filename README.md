# TMS570 ADC2CAN Front ECU

Firmware para la ECU ADC2CAN delantera basada en TI TMS570LS1224. El proyecto toma senales analogicas, aplica conversiones y calibraciones, y publica los datos por CAN para el resto de la red del auto.

El punto de entrada activo es [source/sys_main.c](source/sys_main.c).

## CAN

Todos los frames de datos usan 8 bytes (salvo calibracion). El orden de bytes en las tablas es el orden en el bus: b0 es el primer byte y b7 el ultimo.

### Message boxes

| Box | Tipo | CAN ID | Estructura |
| --- | --- | ---: | --- |
| 1 | RX | `0x600` | `calibration_cmd_t` |
| 2 | TX | `0x501` | `tps_data_t` |
| 3 | TX | `0x502` | `front_data_t` |
| 4 | TX | `0x401` | `implausibility_t` (por evento) |

### Frame recibido

| CAN ID | Estructura | Bytes |
| --- | --- | --- |
| `0x600` | `calibration_cmd_t` | `b0 = cmd_id`, `b1-b7 = libre` |

### Frames transmitidos

| CAN ID | Estructura | Bytes |
| --- | --- | --- |
| `0x501` | `tps_data_t` | `b0-b1 = tps1`, `b2-b3 = tps2`, `b4-b7 = libre` |
| `0x502` | `front_data_t` | `b0-b1 = front_left_speed`, `b2-b3 = front_right_speed`, `b4-b5 = direction_angle`, `b6-b7 = front_brake_pressure` |
| `0x401` | `implausibility_t` | `b0 = TPS implausibility`, `b1 = brake implausibility`, `b2-b7 = 0x00` |

### Frame de calibracion (ID `0x600`)

Condicion de seguridad recomendada:
enviar comandos de calibracion solo con el auto en estado seguro (sin torque habilitado).

`DLC = 1`

| `cmd_id` | Accion en esta placa |
| ---: | --- |
| `0` | Sin accion |
| `1` | Calibracion TPS 0 % (`tps1_max`, `tps2_min`) y freno delantero a 0 PSI |
| `2` | Calibracion TPS 100 % (`tps1_min`, `tps2_max`) |
| `3` | Ignorado (izquierda volante, no implementado) |
| `4` | Calibracion centro volante |
| `5` | Ignorado (derecha volante, no implementado) |
| `6` | Ignorado (sensores de corriente, lo hace la trasera) |
| `7` - `0xE` | Ignorados: son comandos de la main ECU (drive, traccion, modo, telemetria) |

Solo los comandos `1`, `2` y `4` escriben la calibracion en la FEE. Cualquier otro `cmd_id` se descarta sin tocar la flash: la escritura es bloqueante y corre dentro de la interrupcion RTI, asi que hacerla en cada drive on/off podia demorar el envio de `0x501`/`0x502` mas alla del timeout de 100 ms de la main ECU.

## ADC2CAN Delantera

### `0x501`: TPS Data

`DLC = 8`

| Bytes | Tipo | Significado |
| --- | --- | --- |
| `B0 (LSB), B1 (MSB)` | `uint16_t` | TPS1 |
| `B2 (LSB), B3 (MSB)` | `uint16_t` | TPS2 |
| `B4 - B7` | - | Free |

### `0x502`: Front Data

`DLC = 8`

| Bytes | Tipo | Significado |
| --- | --- | --- |
| `B0 (LSB), B1 (MSB)` | `uint16_t` | Front Left Speed |
| `B2 (LSB), B3 (MSB)` | `uint16_t` | Front Right Speed |
| `B4 (LSB), B5 (MSB)` | `uint16_t` | Direction Angle |
| `B6 (LSB), B7 (MSB)` | `uint16_t` | Front Brake Pressure |

### `0x401`: Implausibility

`DLC = 8`. **No es periodico**: se manda una sola vez cuando se confirma una falla.

| Bytes | Tipo | Significado |
| --- | --- | --- |
| `B0` | `uint8_t` | `1` = TPS implausibility (`error_TPS_IMPLAUSIBILITY`), `0` = no |
| `B1` | `uint8_t` | `1` = brake implausibility (`ERROR_BREAK_IMPLAUSIBILITY`), `0` = no |
| `B2 - B7` | - | `0x00` |

La main ECU (`tms570_main_ecu`) ya no detecta implausibilidades: las detectan las placas adc2can. Cuando la main ECU recibe un `0x401` con `B0 != 0` o `B1 != 0` fuerza drive off (igual que el cmd 8 del `0x600`). No guarda la falla: el auto vuelve a andar recien con un drive-on nuevo (cmd 7 por `0x600`).

## Deteccion de implausibilidades

Se evalua en cada ciclo de `convert_data()` (`DATA_PROCESS_HZ = 150 Hz`, ~6.7 ms). Cada falla tiene un contador de ciclos consecutivos: si la condicion desaparece antes de 100 ms el contador se resetea.

| Falla | Byte | Condicion (tiene que persistir > 100 ms) | Reglamento |
| --- | --- | --- | --- |
| TPS | `B0` | `abs(tps1_value - tps2_value) > 10 %` | T.4.2 (APPS) |
| TPS | `B0` | `tps1_raw_value` o `tps2_raw_value` fuera de su rango calibrado +/- 25 % del recorrido (circuito abierto / corto) | T.4.2 (APPS) |
| Freno delantero | `B1` | `front_brake_raw_value` < calibracion - 25 % o > 4.5 V (circuito abierto / corto) | T.4.3.3/4 (BSE) |

- Los chequeos de rango se hacen sobre el valor **RAW** del ADC (filtrado), antes del clamp de `convert_data()`. El TPS en % esta clampeado a 0-100 y la presion de freno se clampea a 0 PSI por debajo de la calibracion, asi que un circuito abierto no se puede detectar desde los valores convertidos.
- Deteccion por flanco: el `0x401` se manda una sola vez cuando la falla se confirma, no un frame por ciclo mientras dure. Cuando la condicion desaparece la deteccion se re-arma y se vuelve a mandar recien si la falla aparece de nuevo.
- Si el mailbox 4 todavia tiene una transmision pendiente, se reintenta en el ciclo siguiente.
- `B0`/`B1` reflejan las fallas confirmadas al momento de mandar el frame.
- Rango RAW del TPS: se arma con los valores de calibracion (`tpsX_min_value`, `tpsX_max_value`) ensanchados un `TPS_RAW_MARGIN_PCT` del recorrido calibrado de cada lado: `[min - 25 % * (max - min), max + 25 % * (max - min)]`. Ejemplo: min = 800, max = 3200 -> rango normal 200 - 3800.
- Rango RAW del freno: el minimo sale de `calibration_brake_value` (RAW a 0 PSI, se guarda con el cmd 1) menos un `BRAKE_RAW_MIN_MARGIN_PCT` de ese valor; el maximo queda fijo en 4.5 V. Ejemplo: calibracion = 500 -> rango normal 375 - 3686. Si el freno no esta calibrado (`calibration_brake_value = 0`) el minimo vuelve a 0.5 V (410).
- Con el TPS sin calibrar (min/max en 0) o con una calibracion invalida (`max <= min`) se reporta implausibility de TPS.

Umbrales (`#define` en [source/sensors.h](source/sensors.h)):

| Define | Valor | Significado |
| --- | ---: | --- |
| `IMPLAUSIBILITY_TIME_MS` | `100` | Persistencia minima de la falla |
| `IMPLAUSIBILITY_CYCLES` | `15` | Persistencia en ciclos de `convert_data()` (calculado) |
| `TPS_IMPLAUSIBILITY_DIFF` | `10` | Desvio maximo TPS1 vs TPS2 [%] |
| `TPS_RAW_MARGIN_PCT` | `25` | Margen sobre el rango calibrado del TPS [% del recorrido `max - min`] |
| `BRAKE_RAW_MIN_MARGIN_PCT` | `25` | Margen por debajo de la calibracion del freno [% de `calibration_brake_value`] |
| `BRAKE_RAW_MIN_UNCALIBRATED` | `410` | Minimo RAW del freno sin calibrar (0.5 V) |
| `BRAKE_RAW_MAX` | `3686` | Maximo RAW del freno (4.5 V) |

## Flujo de ejecucion

- Inicializacion en [source/sys_main.c](source/sys_main.c): `sciInit`, `canInit`, `init_sensors`.
- Procesamiento periodico por RTI compare2: `convert_data` (incluye deteccion de implausibilidades y envio del `0x401`) y `process_calibration_command`.
- Envio periodico por RTI compare3: `send_data_to_serial` y `send_data_to_can`.
- Recepcion CAN en [source/sys_main.c](source/sys_main.c): `canMessageNotification` para mailbox 1 (`0x600`).

## Variables y conversiones

- Sensores y calibraciones en [source/sensors.h](source/sensors.h): `sensors_data_t`, `calibration_cmd_id_t`.
- Logica de conversion y empaquetado CAN en [source/sensors.c](source/sensors.c).
- Conversores activos:
	- TPS1/TPS2 en porcentaje (`0..100`).
	- Presion de freno delantera en PSI (segun ecuacion del sensor).
	- Angulo de direccion en escala interna (`direction_value`).
	- Velocidad de rueda izquierda/derecha en RPM.

## Resumen corto

- `0x600` (RX, DLC 1): comando de calibracion.
- `0x501` (TX, DLC 8): TPS1 y TPS2.
- `0x502` (TX, DLC 8): velocidades delanteras, direccion y presion de freno delantera.
- `0x401` (TX, DLC 8, por evento): implausibilidad de TPS (`B0`) y freno delantero (`B1`).