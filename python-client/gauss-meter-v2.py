import csv
import sys
import serial.tools.list_ports
import time

from PySide6.QtWidgets import QApplication, QMainWindow, QWidget, QMessageBox
from PySide6.QtWidgets import QLabel, QComboBox, QPushButton, QLCDNumber, QHBoxLayout, QVBoxLayout, QSizePolicy, QCheckBox, QLineEdit, QFileDialog
from PySide6.QtCore import QTime, QTimer, QObject, QThread, Signal, Slot

class MainWindow(QMainWindow):
    state = "setting"   # setting, sending, active, pause

    port_list = []
    port_str_list = []
    selected_port = None

    sampling_rate = 0
    range = 0
    multiplier = [75 / 32768, 150 / 32768, 300 / 32768]
    busy_setting = False
    busy_measuring = False
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
        self.reset_button.setEnabled(False)
        self.reset_button.clicked.connect(self.reset_state)

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
        self.max_measurements = 1000  # default to 1000 if user does not specify

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
        self.timer.start(1000)  # update every second
    
    def browse_file(self):
        # prompt user for save filename
        path, _ = QFileDialog.getSaveFileName(self, "Select file to save to", "", "CSV Files (*.csv);;All Files (*)")
        if path:
            self.file_lineedit.setText(path)
            self.save_file = path

    def update_things(self):
        if self.busy_measuring:
            return

        # update the list of available ports
        self.port_list.clear()
        self.port_str_list.clear()
        ports = serial.tools.list_ports.comports()
        for port, desc, hwid in sorted(ports):
            if desc != "n/a":
                self.port_str_list.append(f"{port}: {desc}")
                self.port_list.append(port)
        
        # update combo box with new list
        self.port_cbx.blockSignals(True)        # block selection signals when updating list
        current_text = self.port_cbx.currentText()
        self.port_cbx.clear()
        self.port_cbx.addItems(self.port_str_list)

        if current_text in self.port_str_list:  # if the selected port still exists
            index = self.port_str_list.index(current_text)
            self.port_cbx.setCurrentIndex(index)    # selection to the new index of the same port
        else:                                   # if the selected port doesn't exist
            if self.selected_port is not None:  # if the selected port is active
                self.selected_port.close()      # close the selected port
                self.selected_port = None
            self.port_cbx.setCurrentIndex(-1)   # don't select any port
        self.port_cbx.blockSignals(False)       # unblock selection signals
    
    def port_cbx_changed(self, index):
        self.port_name = self.port_list[index]
    
    def sr_cbx_changed(self, index):
        self.sampling_rate = int(index)
    
    def range_cbx_changed(self, index):
        self.range = int(index)
    
    @Slot(list)
    def update_lcd(self, data_list):
        Bx, By, Bz = data_list

        self.lcdx.display(f"{Bx:.01f}")
        self.lcdy.display(f"{By:.01f}")
        self.lcdz.display(f"{Bz:.01f}")

    def start_logging(self):
        # read max measurements if provided, otherwise default to 1000
        if self.max_lineedit.text().strip():
            try:
                self.max_measurements = int(self.max_lineedit.text())
            except ValueError:
                QMessageBox.warning(self, "Input Error", "Max measurements must be an integer.")
                return
        else:
            self.max_measurements = 1000

        # disable inputs to prevent changes during logging
        self.port_cbx.setEnabled(False)
        self.sr_cbx.setEnabled(False)
        self.range_cbx.setEnabled(False)
        self.save_checkbox.setEnabled(False)
        self.file_lineedit.setEnabled(False)
        self.browse_button.setEnabled(False)
        self.max_lineedit.setEnabled(False)
        self.start_button.setEnabled(False)
        self.reset_button.setEnabled(True)
        
        # start thread for receiving serial port data
        self.thread = QThread()
        if self.save_checkbox.isChecked():
            file_name = self.save_file
        else:
            file_name = None
        self.worker = SerialWorker(self.port_name, file_name, 115200, self.sampling_rate, self.range, self.max_measurements)
        self.worker.moveToThread(self.thread)

        self.thread.started.connect(self.worker.run)
        self.worker.clock.connect(self.update_lcd)
        self.worker.finished.connect(self.thread.quit)
        self.worker.finished.connect(self.reset_state)
        self.worker.finished.connect(self.worker.deleteLater)
        self.thread.finished.connect(self.thread.deleteLater)

        self.busy_measuring = True
        self.thread.start()
    
    def reset_state(self):
        if self.worker:
            self.worker.stop()
        if self.thread:
            self.thread.quit()
            self.thread.wait(1000)

        self.busy_measuring = False

        # enable inputs after reset
        self.port_cbx.setEnabled(True)
        self.sr_cbx.setEnabled(True)
        self.range_cbx.setEnabled(True)
        self.save_checkbox.setEnabled(True)
        self.file_lineedit.setEnabled(True)
        self.browse_button.setEnabled(True)
        self.max_lineedit.setEnabled(True)
        self.start_button.setEnabled(True)
        self.reset_button.setEnabled(False)


class SerialWorker(QObject):
    data_received = Signal(list)
    finished = Signal()
    clock = Signal(list)

    latest_data = [0, 0, 0]

    def __init__(self, port_name, file_name, buadrate=115200, sampling_rate=0, measuring_range=0, max_measurements=1000):
        super().__init__()
        self.buffer = []
        self.csv_batch_size = 10
        self.serial_batch_size = 64
        self.frame_size = 12
        self.multiplier = [75 / 32768, 150 / 32768, 300 / 32768]

        self.port_name = port_name
        self.file_name = file_name
        self.buadrate = buadrate
        self.sampling_rate = sampling_rate
        self.measuring_range = measuring_range
        self.max_measurements = max_measurements
        self._stop_requested = False

        # We emit display updates from within the serial read loop.
        # A QTimer would not fire reliably while run() blocks the thread's event loop.
    
    def stop(self):
        self._stop_requested = True

    @Slot()
    def run(self):
        f = None
        writer = None
        try:
            with serial.Serial(self.port_name, self.buadrate, timeout=0.1) as ser:
                if self.file_name:
                    f = open(self.file_name, "w", newline="")
                    writer = csv.writer(f)
                    writer.writerow(["time", "Bx", "By", "Bz"])

                # write 7-byte setting command to STM32
                ser.reset_input_buffer()
                bytes_to_write = self.sampling_rate.to_bytes(1, "little")
                setting_command = b"H" + bytes_to_write
                bytes_to_write = self.measuring_range.to_bytes(1, "little")
                setting_command += bytes_to_write
                bytes_to_write = self.max_measurements.to_bytes(4, "little")
                setting_command += bytes_to_write
                ser.write(setting_command)
                print("Setting Sent.")

                # wait for confirmation from STM32
                bytes_read = b"0"
                while bytes_read != b"D" and not self._stop_requested:
                    bytes_read = ser.read(1)
                    time.sleep(0.01)
                print("Setting Completed.")
                ser.reset_input_buffer()

                i_batch = 0
                expected_bytes = min(self.max_measurements, self.serial_batch_size) * self.frame_size + 1
                while not self._stop_requested and i_batch * self.serial_batch_size < self.max_measurements:
                    while not self._stop_requested and ser.in_waiting < expected_bytes:
                        time.sleep(0.001)

                    if self._stop_requested:
                        ser.write(b"S")
                        break

                    print(f"ser.in_waiting={ser.in_waiting}")
                    raw_data = ser.read(expected_bytes)
                    if len(raw_data) < expected_bytes:
                        print(f"Short read ({len(raw_data)} bytes), stopping.")
                        break

                    # +1 for block length info at 0th byte
                    block_length = raw_data[0]
                    print(f"block_length={block_length}")

                    for i in range(block_length):
                        block_begin = i * self.frame_size + 1       # block begin index in buffer
                        t1 = int.from_bytes(raw_data[block_begin : block_begin + 4], "little")
                        Bx = int.from_bytes(raw_data[block_begin + 4 : block_begin + 6], "little", signed=True) * self.multiplier[self.measuring_range]
                        By = int.from_bytes(raw_data[block_begin + 6 : block_begin + 8], "little", signed=True) * self.multiplier[self.measuring_range]
                        Bz = int.from_bytes(raw_data[block_begin + 8 : block_begin + 10], "little", signed=True) * self.multiplier[self.measuring_range]
                        self.buffer.append([f"{t1}", f"{Bx:.2f}", f"{By:.2f}", f"{Bz:.2f}"])
                        self.latest_data = [Bx, By, Bz]
                        self.clock.emit(self.latest_data)

                    i_batch += 1

                    # write to CSV file if enabled
                    if writer and i_batch % self.csv_batch_size == 0:
                        writer.writerows(self.buffer)
                        self.buffer.clear()
                    
                    left_bytes = self.max_measurements - i_batch * self.serial_batch_size
                    if left_bytes > self.serial_batch_size:
                        expected_bytes = self.serial_batch_size * self.frame_size + 1
                    else:       # if the last batch is shorter than the serial batch size
                        expected_bytes = left_bytes * self.frame_size + 1
                    
                    # print(f"expected_bytes={expected_bytes}")
                    # print(f"i_batch*serial_batch_size={i_batch*self.serial_batch_size}")
                
                if writer and len(self.buffer):
                    writer.writerows(self.buffer)
                    self.buffer.clear()

        except Exception as e:
            QMessageBox.critical(None, "Port Error", "Something went wrong. Please check port selection.")
        finally:
            if f:
                f.close()
            self.finished.emit()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()

    sys.exit(app.exec())