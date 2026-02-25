import sys
import serial.tools.list_ports
import time

from PySide6.QtWidgets import QApplication, QMainWindow, QWidget
from PySide6.QtWidgets import QComboBox, QLCDNumber, QHBoxLayout, QVBoxLayout, QSizePolicy
from PySide6.QtCore import QTime, QTimer

class MainWindow(QMainWindow):

    port_list = []
    port_str_list = []
    selected_port = None

    def __init__(self):
        super().__init__()

        self.setWindowTitle("Test")

        self.combo_box = QComboBox(self)
        self.combo_box.currentIndexChanged.connect(self.combo_box_changed)

        self.lcdx = QLCDNumber(self)
        self.lcdx.setDigitCount(6)
        self.lcdx.setSegmentStyle(QLCDNumber.Flat)
        self.lcdx.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.lcdx.setMinimumSize(200, 80)
        self.lcdx.setMaximumSize(400, 160)

        self.lcdy = QLCDNumber(self)
        self.lcdy.setDigitCount(6)
        self.lcdy.setSegmentStyle(QLCDNumber.Flat)
        self.lcdy.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.lcdy.setMinimumSize(200, 80)
        self.lcdy.setMaximumSize(400, 160)

        self.lcdz = QLCDNumber(self)
        self.lcdz.setDigitCount(6)
        self.lcdz.setSegmentStyle(QLCDNumber.Flat)
        self.lcdz.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.lcdz.setMinimumSize(200, 80)
        self.lcdz.setMaximumSize(400, 160)

        self.lcd_container = QWidget()
        self.lcd_layout = QHBoxLayout(self.lcd_container)
        self.lcd_layout.addStretch()  # Add space on the left
        self.lcd_layout.addWidget(self.lcdx)
        self.lcd_layout.addWidget(self.lcdy)
        self.lcd_layout.addWidget(self.lcdz)
        self.lcd_layout.addStretch()  # Add space on the right

        self.container = QWidget()
        self.layout = QVBoxLayout(self.container)
        self.layout.addWidget(self.combo_box)
        self.layout.addWidget(self.lcd_container)

        self.setCentralWidget(self.container)

        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_things)
        self.timer.start(500)  # update every half a second
    
    def update_things(self):
        # update the list of available ports
        self.port_list.clear()
        self.port_str_list.clear()
        ports = serial.tools.list_ports.comports()
        for port, desc, hwid in sorted(ports):
            if desc != "n/a":
                self.port_str_list.append(f"{port}: {desc}")
                self.port_list.append(port)
        
        # update combo box with new list
        self.combo_box.blockSignals(True)
        current_text = self.combo_box.currentText()
        self.combo_box.clear()
        self.combo_box.addItems(self.port_str_list)
        if current_text in self.port_str_list:
            index = self.port_str_list.index(current_text)
            self.combo_box.setCurrentIndex(index)
        else:
            if self.selected_port is not None:
                self.selected_port.close()
                self.selected_port = None
            self.combo_box.setCurrentIndex(-1)
        self.combo_box.blockSignals(False)

        # update measurement
        if self.selected_port is not None:
            # send measure command
            num_measure = 1
            bytes_to_write = num_measure.to_bytes(4, "little")
            self.selected_port.write(b"S")
            self.selected_port.write(bytes_to_write)

            # wait for response
            # while self.selected_port.in_waiting < 10:
            #     time.sleep(0.01)
            
            # read response from serial port
            bytes_read = self.selected_port.read(10)
            if len(bytes_read) == 10:
                # convert data from bytes to int and apply adequate scaling for magnetic field values
                t1 = int.from_bytes(bytes_read[0:4], "little")
                Bx = int.from_bytes(bytes_read[4:6], "little", signed=True) * 300 / 32768
                By = int.from_bytes(bytes_read[6:8], "little", signed=True) * 300 / 32768
                Bz = int.from_bytes(bytes_read[8:10], "little", signed=True) * 300 / 32768
                print(f"{t1}: {Bx}, {By}, {Bz}")

                self.lcdx.display(f"{Bx:.01f}")
                self.lcdy.display(f"{By:.01f}")
                self.lcdz.display(f"{Bz:.01f}")
            else:
                print("Something went wrong. Please check port selection.")
            self.selected_port.flush()
    
    def combo_box_changed(self, index):
        print(f"Selected: {self.port_list[index]}")
        self.selected_port = serial.Serial(self.port_list[index], baudrate=115200, timeout=0.05)
        # self.selected_port.close()
        
if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()

    sys.exit(app.exec())