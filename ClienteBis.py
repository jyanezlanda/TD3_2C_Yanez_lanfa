import socket
import struct
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np
import time

# Configuración inicial
HOST = '192.168.1.5'  # Dirección IP del servidor
PORT = 8081        # Puerto del servidor
BUFFER_SIZE = 1024  # Tamaño del buffer

# Parámetros de gráficos
variable_names = ["AccelX", "AccelY", "AccelZ", "GyroX", "GyroY", "GyroZ", "Temp"]
num_variables = len(variable_names)
window_size = 100

# Inicializar datos para las gráficas
raw_data = np.zeros((num_variables, window_size))
avg_data = np.zeros((num_variables, window_size))
filtered_data = np.zeros((num_variables, window_size))

# Crear subgráficas
fig, axes = plt.subplots(num_variables, 1, figsize=(10, 15))
lines_raw = []
lines_avg = []
lines_filtered = []

for i, ax in enumerate(axes):
    ax.set_xlim(0, window_size)
    ax.set_title(variable_names[i])
    ax.grid(True)
    
    # Líneas para datos crudos, promediados y filtrados
    line_raw, = ax.plot([], [], label='Crudo', color='blue')
    line_avg, = ax.plot([], [], label='Promediado', color='orange')
    line_filtered, = ax.plot([], [], label='Filtrado (EWMA)', color='green')
    lines_raw.append(line_raw)
    lines_avg.append(line_avg)
    lines_filtered.append(line_filtered)
    
    # Configurar la leyenda fuera del área del gráfico
    ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', borderaxespad=0.)

# Control de tiempo para solicitudes
last_request_time = 0

# Función para actualizar los datos en tiempo real
def update(frame):
    global raw_data, avg_data, filtered_data, last_request_time
    
    try:
        # Enviar solicitud solo si han pasado al menos 2 segundos
        current_time = time.time()
        if current_time - last_request_time >= 1:
            client.sendall(b"KA")
            data = client.recv(BUFFER_SIZE)
            last_request_time = current_time

            if not data:
                return

            # Dividir datos recibidos y convertirlos a float
            received_values = list(map(float, data.decode().split('\n')[:-1]))
            raw_values = np.array(received_values[:7])
            avg_values = np.array(received_values[7:14])
            filtered_values = np.array(received_values[14:])
            
            # Actualizar datos en las ventanas de graficación
            raw_data = np.roll(raw_data, -1, axis=1)
            avg_data = np.roll(avg_data, -1, axis=1)
            filtered_data = np.roll(filtered_data, -1, axis=1)
            
            raw_data[:, -1] = raw_values
            avg_data[:, -1] = avg_values
            filtered_data[:, -1] = filtered_values

        # Actualizar líneas de las gráficas
        for i in range(num_variables):
            lines_raw[i].set_data(np.arange(window_size), raw_data[i])
            lines_avg[i].set_data(np.arange(window_size), avg_data[i])
            lines_filtered[i].set_data(np.arange(window_size), filtered_data[i])
            
            # Actualizar límites dinámicamente
            all_data = np.concatenate((raw_data[i], avg_data[i], filtered_data[i]))
            min_val, max_val = np.min(all_data), np.max(all_data)
            padding = (max_val - min_val) * 0.1  # Agregar un 10% de margen
            axes[i].set_ylim(min_val - padding, max_val + padding)
    except Exception as e:
        print(f"Error: {e}")

# Configuración del socket
client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect((HOST, PORT))
print(f'Conectado al servidor {HOST}:{PORT}')

# Protocolo de conexión
init_response = client.recv(BUFFER_SIZE).decode()
if init_response != "OK":
    print("Error: No se recibió el mensaje de inicio esperado.")
    client.close()
    exit()

client.sendall(b"AKN")
init_response = client.recv(BUFFER_SIZE).decode()
if init_response != "OK":
    print("Error: No se pudo establecer la conexión con el servidor.")
    client.close()
    exit()

print("Conexión establecida con éxito.")

# Animación en tiempo real
ani = FuncAnimation(fig, update, interval=50)

plt.tight_layout()
plt.subplots_adjust(right=0.8)  # Dejar espacio a la derecha para las leyendas
plt.show()

# Finalizar conexión
client.sendall(b"END")
client.close()