import serial

# Configura la conexión serial
ser = serial.Serial('/dev/ttyUSB0', 115200)  # Ajusta el puerto serial según corresponda (por ejemplo, en Windows podría ser 'COM3')

while True:
    if ser.in_waiting > 0:
        mensaje = ser.readline().decode('utf-8').rstrip()  # Lee la línea y decodifica desde bytes a string UTF-8
        print(f'Mensaje recibido desde Arduino: {mensaje}')
