import sys
import serial.tools.list_ports
import time

from PySide6.QtWidgets import QApplication, QMainWindow, QWidget, QMessageBox
from PySide6.QtWidgets import QLabel, QComboBox, QPushButton, QLCDNumber, QHBoxLayout, QVBoxLayout, QSizePolicy, QCheckBox, QLineEdit, QFileDialog
from PySide6.QtCore import QTime, QTimer

class MainWindow(QMainWindow):
    state = "setting"   # setting, sending, active, pause

    port_list = []
    port_str_list = []
    selected_port = None

    sampling_rate = 0
    range = 0
    multiplier = [75 / 32768, 150 / 32768, 300 / 32768]
    busy_setting = False
    error_flag = False
    error_flag_prev = False

    def __init__(self):
        super().__init__()

        self.setWindowTitle("USB Gauss Meter")

        # Serial port selection combo box
        self.port_cbx_label = QLabel(self)
        self.port_cbx_label.setText("Please select a serial device.")
        self.port_cbx = QComboBox(self)
        self.port_cbx.currentIndexChanged.connect(self.port_cbx_changed)

        # Sampling rate selection combo box
        self.sr_cbx_label = QLabel(self)
        self.sr_cbx_label.setText("Select sampling rate for logging:")
        self.sr_cbx = QComboBox(self)
        self.sr_cbx.addItems(['10ksps', '5.7ksps', '3.1ksps', '1.6ksps', '0.8ksps', '0.4ksps'])
        self.sr_cbx.currentIndexChanged.connect(self.sr_cbx_changed)

        # Range selection combo box
        self.range_cbx_label = QLabel(self)
        self.range_cbx_label.setText("Select measurement range:")
        self.range_cbx = QComboBox(self)
        self.range_cbx.addItems(["75mT", "150mT", "300mT"])
        self.range_cbx.currentIndexChanged.connect(self.range_cbx_changed)

        # start, pause, reset buttons
        self.start_button = QPushButton("Start")
        self.start_button.clicked.connect(self.start_logging)
        self.pause_button = QPushButton("Pause")
        self.reset_button = QPushButton("Reset")

        # layout the three buttons in a horizontal row
        self.button_container = QWidget()
        self.button_layout = QHBoxLayout(self.button_container)
        self.button_layout.addStretch()  # Add space on the left
        self.button_layout.addWidget(self.start_button)
        self.button_layout.addWidget(self.pause_button)
        self.button_layout.addWidget(self.reset_button)
        self.button_layout.addStretch()  # Add space on the right

        # initialize save file path and max measurements
        self.save_file = None
        self.max_measurements = None

        # LCD displays for Bx, By, Bz
        self.lcdx_label = QLabel(self)
        self.lcdx_label.setText("Bx (mT):")
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
        self.lcdy_label.setText("By (mT):")
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
        self.lcdz_label.setText("Bz (mT):")
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

        # layout three LCDs in a horizontal row
        self.lcd_container = QWidget()
        self.lcd_layout = QHBoxLayout(self.lcd_container)
        self.lcd_layout.addStretch()  # Add space on the left
        self.lcd_layout.addWidget(self.lcdx_container)
        self.lcd_layout.addWidget(self.lcdy_container)
        self.lcd_layout.addWidget(self.lcdz_container)
        self.lcd_layout.addStretch()  # Add space on the right

        # layout of application
        self.container = QWidget()
        self.layout = QVBoxLayout(self.container)
        self.layout.addWidget(self.port_cbx_label)
        self.layout.addWidget(self.port_cbx)
        self.layout.addWidget(self.sr_cbx_label)
        self.layout.addWidget(self.sr_cbx)
        self.layout.addWidget(self.range_cbx_label)
        self.layout.addWidget(self.range_cbx)

        # save options row: checkbox + file selector
        self.save_checkbox = QCheckBox("Save results")
        self.file_lineedit = QLineEdit(self)
        self.file_lineedit.setPlaceholderText("Select file...")
        self.browse_button = QPushButton("Browse")
        self.browse_button.clicked.connect(self.browse_file)

        self.save_container = QWidget()
        self.save_layout = QHBoxLayout(self.save_container)
        self.save_layout.addWidget(self.save_checkbox)
        self.save_layout.addWidget(self.file_lineedit)
        self.save_layout.addWidget(self.browse_button)
        self.layout.addWidget(self.save_container)

        # maximum measurements input
        self.max_label = QLabel("Max number of measurements:")
        self.max_lineedit = QLineEdit(self)
        self.max_lineedit.setPlaceholderText("e.g. 1000")
        self.max_container = QWidget()
        self.max_layout = QHBoxLayout(self.max_container)
        self.max_layout.addWidget(self.max_label)
        self.max_layout.addWidget(self.max_lineedit)
        self.layout.addWidget(self.max_container)

        self.layout.addWidget(self.button_container)
        self.layout.addWidget(self.lcd_container)

        self.setCentralWidget(self.container)

        # display update timer
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_things)
        self.timer.start(500)  # update every half a second
    
    def browse_file(self):
        # prompt user for save filename
        path, _ = QFileDialog.getSaveFileName(self, "Select file to save to", "", "CSV Files (*.csv);;All Files (*)")
        if path:
            self.file_lineedit.setText(path)
            self.save_file = path

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
    
    def port_cbx_changed(self, index):
        print(f"Selected: {self.port_list[index]}")
        self.selected_port = serial.Serial(self.port_list[index], baudrate=115200, timeout=0.05)
    
    def sr_cbx_changed(self, index):
        self.sampling_rate = int(index)
    
    def range_cbx_changed(self, index):
        self.range = int(index)
    
    def start_logging(self):
        if self.selected_port is not None:
            # if user requested saving, attempt to open file
            if self.save_checkbox.isChecked() and self.save_file:
                try:
                    self.output_handle = open(self.save_file, "w")
                    # optionally write header
                    self.output_handle.write("timestamp,bx,by,bz\n")
                except Exception as e:
                    QMessageBox.warning(self, "File Error", f"Could not open file: {e}")
            
            # read max measurements if provided
            if self.max_lineedit.text().strip():
                try:
                    self.max_measurements = int(self.max_lineedit.text())
                except ValueError:
                    QMessageBox.warning(self, "Input Error", "Max measurements must be an integer.")
                    return

            # disable inputs to prevent changes during logging
            self.port_cbx.setEnabled(False)
            self.sr_cbx.setEnabled(False)
            self.range_cbx.setEnabled(False)
            self.save_checkbox.setEnabled(False)
            self.file_lineedit.setEnabled(False)
            self.browse_button.setEnabled(False)
            self.max_lineedit.setEnabled(False)
            self.start_button.setEnabled(False)
            self.state = "sending"

            # setting sampling rate
            # bytes_to_write = self.sampling_rate.to_bytes(1, "little")
            # self.selected_port.write(b"H" + bytes_to_write)

            # wait for confirmation from STM32
            # bytes_read = b"0"
            # while bytes_read != b"D":
            #     bytes_read = self.selected_port.read(1)
            #     time.sleep(0.01)

            # setting range
            bytes_to_write = self.range.to_bytes(1, "little")
            self.selected_port.write(b"R" + bytes_to_write)

            # wait for confirmation from STM32
            bytes_read = b"0"
            while bytes_read != b"D":
                bytes_read = self.selected_port.read(1)
                time.sleep(0.01)

            self.selected_port.flush()
            self.state = "active"
        else:
            QMessageBox.critical(None, "Port Error", "Something went wrong. Please check port selection.")

        
if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()

    sys.exit(app.exec())