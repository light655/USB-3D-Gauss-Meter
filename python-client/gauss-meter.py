import sys
import serial.tools.list_ports
import time

from PySide6.QtWidgets import QApplication, QMainWindow, QWidget
from PySide6.QtWidgets import QLabel, QComboBox, QLCDNumber, QHBoxLayout, QVBoxLayout, QSizePolicy
from PySide6.QtCore import QTime, QTimer

class MainWindow(QMainWindow):

    port_list = []
    port_str_list = []
    selected_port = None

    range = 0
    multiplier = [75 / 32768, 150 / 32768, 300 / 32768]
    busy_setting = False

    def __init__(self):
        super().__init__()

        self.setWindowTitle("USB Gauss Meter")

        # Serial port selection combo box
        self.port_cbx_label = QLabel(self)
        self.port_cbx_label.setText("Please select a serial device.")
        self.port_cbx = QComboBox(self)
        self.port_cbx.currentIndexChanged.connect(self.port_cbx_changed)

        # Range selection combo box
        self.range_cbx_label = QLabel(self)
        self.range_cbx_label.setText("Select measurement range:")
        self.range_cbx = QComboBox(self)
        self.range_cbx.addItems(["75mT", "150mT", "300mT"])
        self.range_cbx.currentIndexChanged.connect(self.range_cbx_changed)

        # LCD displays for Bx, By, Bz
        self.lcdx_label = QLabel(self)
        self.lcdx_label.setText("Bx:")
        self.lcdx = QLCDNumber(self)
        self.lcdx.setDigitCount(6)
        self.lcdx.setSegmentStyle(QLCDNumber.Flat)
        self.lcdx.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.lcdx.setMinimumSize(200, 80)
        self.lcdx.setMaximumSize(400, 160)
        self.lcdx_container = QWidget()
        self.lcdx_layout = QVBoxLayout(self.lcdx_container)
        self.lcdx_layout.addStretch()
        self.lcdx_layout.addWidget(self.lcdx_label)
        self.lcdx_layout.addWidget(self.lcdx)
        self.lcdx_layout.addStretch()

        self.lcdy_label = QLabel(self)
        self.lcdy_label.setText("By:")
        self.lcdy = QLCDNumber(self)
        self.lcdy.setDigitCount(6)
        self.lcdy.setSegmentStyle(QLCDNumber.Flat)
        self.lcdy.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.lcdy.setMinimumSize(200, 80)
        self.lcdy.setMaximumSize(400, 160)
        self.lcdy_container = QWidget()
        self.lcdy_layout = QVBoxLayout(self.lcdy_container)
        self.lcdy_layout.addStretch()
        self.lcdy_layout.addWidget(self.lcdy_label)
        self.lcdy_layout.addWidget(self.lcdy)
        self.lcdy_layout.addStretch()

        self.lcdz_label = QLabel(self)
        self.lcdz_label.setText("Bz:")
        self.lcdz = QLCDNumber(self)
        self.lcdz.setDigitCount(6)
        self.lcdz.setSegmentStyle(QLCDNumber.Flat)
        self.lcdz.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.lcdz.setMinimumSize(200, 80)
        self.lcdz.setMaximumSize(400, 160)
        self.lcdz_container = QWidget()
        self.lcdz_layout = QVBoxLayout(self.lcdz_container)
        self.lcdz_layout.addStretch()
        self.lcdz_layout.addWidget(self.lcdz_label)
        self.lcdz_layout.addWidget(self.lcdz)
        self.lcdz_layout.addStretch()

        self.lcd_container = QWidget()
        self.lcd_layout = QHBoxLayout(self.lcd_container)
        self.lcd_layout.addStretch()  # Add space on the left
        self.lcd_layout.addWidget(self.lcdx_container)
        self.lcd_layout.addWidget(self.lcdy_container)
        self.lcd_layout.addWidget(self.lcdz_container)
        self.lcd_layout.addStretch()  # Add space on the right

        self.container = QWidget()
        self.layout = QVBoxLayout(self.container)
        self.layout.addWidget(self.port_cbx_label)
        self.layout.addWidget(self.port_cbx)
        self.layout.addWidget(self.range_cbx_label)
        self.layout.addWidget(self.range_cbx)
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
        self.port_cbx.blockSignals(True)
        current_text = self.port_cbx.currentText()
        self.port_cbx.clear()
        self.port_cbx.addItems(self.port_str_list)
        if current_text in self.port_str_list:
            index = self.port_str_list.index(current_text)
            self.port_cbx.setCurrentIndex(index)
        else:
            if self.selected_port is not None:
                self.selected_port.close()
                self.selected_port = None
            self.port_cbx.setCurrentIndex(-1)
        self.port_cbx.blockSignals(False)

        # update measurement
        if self.selected_port is not None and self.busy_setting is not True:
            # send measure command
            num_measure = 1
            bytes_to_write = num_measure.to_bytes(4, "little")
            self.selected_port.write(b"S" + bytes_to_write)
            # self.selected_port.write(bytes_to_write)

            # wait for response
            # while self.selected_port.in_waiting < 10:
            #     time.sleep(0.01)
            
            # read response from serial port
            bytes_read = self.selected_port.read(10)
            if len(bytes_read) == 10:
                # convert data from bytes to int and apply adequate scaling for magnetic field values
                t1 = int.from_bytes(bytes_read[0:4], "little")
                Bx = int.from_bytes(bytes_read[4:6], "little", signed=True) * self.multiplier[self.range]
                By = int.from_bytes(bytes_read[6:8], "little", signed=True) * self.multiplier[self.range]
                Bz = int.from_bytes(bytes_read[8:10], "little", signed=True) * self.multiplier[self.range]
                print(f"{t1}: {Bx}, {By}, {Bz}")

                self.lcdx.display(f"{Bx:.01f}")
                self.lcdy.display(f"{By:.01f}")
                self.lcdz.display(f"{Bz:.01f}")
            else:
                print("Something went wrong. Please check port selection.")
            self.selected_port.flush()
    
    def port_cbx_changed(self, index):
        print(f"Selected: {self.port_list[index]}")
        self.selected_port = serial.Serial(self.port_list[index], baudrate=115200, timeout=0.05)
        # self.selected_port.close()
    
    def range_cbx_changed(self, index):
        self.range = int(index)
        
        if self.selected_port is not None:
            self.busy_setting = True        # flag to pause measuring during setting

            bytes_to_write = self.range.to_bytes(1, "little")
            self.selected_port.write(b"R" + bytes_to_write)

            bytes_read = b"0"
            while bytes_read != b"D":
                bytes_read = self.selected_port.read(1)
                print(bytes_read)
                time.sleep(0.01)
            self.busy_setting = False
            self.selected_port.flush()
        
if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()

    sys.exit(app.exec())