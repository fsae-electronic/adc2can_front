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

### Frame recibido

| CAN ID | Estructura | Bytes |
| --- | --- | --- |
| `0x600` | `calibration_cmd_t` | `b0 = cmd_id`, `b1-b7 = libre` |

### Frames transmitidos

| CAN ID | Estructura | Bytes |
| --- | --- | --- |
| `0x501` | `tps_data_t` | `b0-b1 = tps1`, `b2-b3 = tps2`, `b4-b7 = libre` |
| `0x502` | `front_data_t` | `b0-b1 = front_left_speed`, `b2-b3 = front_right_speed`, `b4-b5 = direction_angle`, `b6-b7 = front_brake_pressure` |

### Frame de calibracion (ID `0x600`)

Condicion de seguridad recomendada:
enviar comandos de calibracion solo con el auto en estado seguro (sin torque habilitado).

`DLC = 1`

| `cmd_id` | Accion |
| ---: | --- |
| `0` | Sin accion |
| `1` | Calibracion TPS 0% |
| `2` | Calibracion TPS 100% |
| `3` | Calibracion izquierda volante |
| `4` | Calibracion centro volante |
| `5` | Calibracion derecha volante |
| `6` | Calibracion sensores de corriente |

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

## Flujo de ejecucion

- Inicializacion en [source/sys_main.c](source/sys_main.c): `sciInit`, `canInit`, `init_sensors`.
- Procesamiento periodico por RTI compare2: `convert_data` y `process_calibration_command`.
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