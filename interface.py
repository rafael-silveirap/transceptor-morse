import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import serial
import serial.tools.list_ports
import threading
import unicodedata
import time


DICIONARIO_MORSE = {
    'A': '.-', 'B': '-...', 'C': '-.-.', 'D': '-..', 'E': '.', 'F': '..-.',
    'G': '--.', 'H': '....', 'I': '..', 'J': '.---', 'K': '-.-', 'L': '.-..',
    'M': '--', 'N': '-.', 'O': '---', 'P': '.--.', 'Q': '--.-', 'R': '.-.',
    'S': '...', 'T': '-', 'U': '..-', 'V': '...-', 'W': '.--', 'X': '-..-',
    'Y': '-.--', 'Z': '--..', '0': '-----', '1': '.----', '2': '..---',
    '3': '...--', '4': '....-', '5': '.....', '6': '-....', '7': '--...',
    '8': '---..', '9': '----.'
}

def texto_para_morse(texto):
    """Remove acentos, passa para maiúsculo e traduz para Morse."""
    texto_limpo = ''.join(c for c in unicodedata.normalize('NFD', texto) 
                          if unicodedata.category(c) != 'Mn')
    texto_limpo = texto_limpo.upper()
    
    palavras_morse = []
    for palavra in texto_limpo.split():
        letras_morse = []
        for letra in palavra:
            if letra in DICIONARIO_MORSE:
                letras_morse.append(DICIONARIO_MORSE[letra])
        palavras_morse.append(' '.join(letras_morse))
        
    return ' / '.join(palavras_morse)


class MorseApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Transceptor Morse - STM32")
        self.root.geometry("650x450")
        
        self.serial_port = None
        self.is_connected = False
        self.baudrate = 115200

        self.setup_ui()
        self.atualizar_lista_portas() # Busca as portas logo ao abrir

        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)

    def setup_ui(self):
        # --- Frame Superior (Conexão) ---
        frame_conexao = tk.Frame(self.root, pady=10)
        frame_conexao.pack(fill=tk.X, padx=10)

        tk.Label(frame_conexao, text="Porta Serial:").pack(side=tk.LEFT)
        
        # Menu Drop-down para as portas
        self.combo_porta = ttk.Combobox(frame_conexao, width=45)
        self.combo_porta.pack(side=tk.LEFT, padx=5)

        # Botão para recarregar a lista de portas
        self.btn_atualizar = tk.Button(frame_conexao, text="↻", command=self.atualizar_lista_portas)
        self.btn_atualizar.pack(side=tk.LEFT, padx=(0, 10))

        self.btn_conectar = tk.Button(frame_conexao, text="Conectar", command=self.toggle_conexao, bg="lightblue")
        self.btn_conectar.pack(side=tk.LEFT, padx=5)

        self.lbl_status = tk.Label(frame_conexao, text="Desconectado", fg="red")
        self.lbl_status.pack(side=tk.RIGHT, padx=10)

        # --- Frame Central (Tela de Mensagens) ---
        frame_chat = tk.Frame(self.root)
        frame_chat.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        self.txt_display = scrolledtext.ScrolledText(frame_chat, wrap=tk.WORD, font=("Courier", 10))
        self.txt_display.pack(fill=tk.BOTH, expand=True)
        self.txt_display.config(state=tk.DISABLED)

        # --- Frame Inferior (Envio) ---
        frame_envio = tk.Frame(self.root, pady=10)
        frame_envio.pack(fill=tk.X, padx=10)

        self.entry_mensagem = tk.Entry(frame_envio, font=("Arial", 12))
        self.entry_mensagem.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 5))
        self.entry_mensagem.bind("<Return>", self.enviar_mensagem)

        self.btn_enviar = tk.Button(frame_envio, text="Enviar", command=self.enviar_mensagem, state=tk.DISABLED)
        self.btn_enviar.pack(side=tk.RIGHT)

    def atualizar_lista_portas(self):
        """Vasculha o computador e atualiza o menu com as portas disponíveis."""
        portas = serial.tools.list_ports.comports()
        lista_portas = [f"{p.device} - {p.description}" for p in portas]

        self.combo_porta['values'] = lista_portas
        
        if lista_portas:
            self.combo_porta.set(lista_portas[0]) # Seleciona a primeira por padrão
        else:
            self.combo_porta.set('')
            self.combo_porta.set('Nenhuma porta encontrada')

    def atualizar_display(self, texto):
        self.txt_display.config(state=tk.NORMAL)
        self.txt_display.insert(tk.END, texto)
        self.txt_display.see(tk.END)
        self.txt_display.config(state=tk.DISABLED)

    def toggle_conexao(self):
        if not self.is_connected:
            # Pega só o nome da porta
            porta = self.combo_porta.get().split(' - ')[0].strip()
            
            if not porta or porta == 'Nenhuma porta encontrada':
                messagebox.showwarning("Aviso", "Selecione uma porta válida antes de conectar.")
                return

            try:
                self.serial_port = serial.Serial(porta, self.baudrate, timeout=0.5)
                time.sleep(1) 
                
                self.is_connected = True
                self.btn_conectar.config(text="Desconectar", bg="lightcoral")
                self.lbl_status.config(text="Conectado", fg="green")
                self.btn_enviar.config(state=tk.NORMAL)
                self.combo_porta.config(state=tk.DISABLED)
                self.btn_atualizar.config(state=tk.DISABLED)
                
                self.atualizar_display(f"--- Conectado na porta {porta} ---\n\n")

                self.thread_leitura = threading.Thread(target=self.ler_serial, daemon=True)
                self.thread_leitura.start()

            except serial.SerialException as e:
                messagebox.showerror("Erro de Conexão", f"Não foi possível abrir a porta {porta}.\n{e}")
        else:
            self.desconectar()

    def desconectar(self):
        self.is_connected = False
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            
        self.btn_conectar.config(text="Conectar", bg="lightblue")
        self.lbl_status.config(text="Desconectado", fg="red")
        self.btn_enviar.config(state=tk.DISABLED)
        self.combo_porta.config(state=tk.NORMAL)
        self.btn_atualizar.config(state=tk.NORMAL)
        self.atualizar_display("\n--- Conexão Encerrada ---\n\n")

    def ler_serial(self):
        while self.is_connected and self.serial_port.is_open:
            try:
                if self.serial_port.in_waiting > 0:
                    dados = self.serial_port.read(self.serial_port.in_waiting)
                    texto = dados.decode('ascii', errors='ignore')
                    self.root.after(0, self.atualizar_display, texto)
            except Exception as e:
                print(f"Erro na leitura: {e}")
                break
            time.sleep(0.05)

    def enviar_mensagem(self, event=None):
        if not self.is_connected:
            return

        texto_usuario = self.entry_mensagem.get().strip()
        if not texto_usuario:
            return

        self.entry_mensagem.delete(0, tk.END)
        string_morse = texto_para_morse(texto_usuario)

        if string_morse:
            self.atualizar_display(f"\n[Você]: {texto_usuario}\n[Enviando Morse]: {string_morse}\n")
            mensagem = string_morse + '\n'
            try:
                self.serial_port.write(mensagem.encode('ascii'))
            except Exception as e:
                messagebox.showerror("Erro de Envio", f"Erro ao enviar dados: {e}")
                self.desconectar()
        else:
            messagebox.showwarning("Aviso", "Texto inválido ou contendo apenas caracteres não suportados.")

    def on_closing(self):
        self.desconectar()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = MorseApp(root)
    root.mainloop()