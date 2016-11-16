import sys
import threading
import serial, time
import serial.tools.list_ports
from ui_lsmod import Ui_Lsmod
from PyQt4 import QtCore, QtGui

def isint(value):
    try:
        int(value)
        return True
    except ValueError:
        return False

class MainWindow(QtGui.QMainWindow):
    ui = Ui_Lsmod()
    ser = serial.Serial()
    tim = QtCore.QTimer()
    timPeriodMs = 100
    get = QtCore.QTimer()
    getPeriodMs = 500
    triggerTestStatus = 0
    
    def __init__(self):
        super(MainWindow, self).__init__()
        self.ui.setupUi(self)
        group = QtGui.QActionGroup(self.ui.menuPort, exclusive = True)
        self.signalMapper = QtCore.QSignalMapper(self)
        self.signalMapper.mapped[str].connect(self.setPort)
        for port in list(serial.tools.list_ports.comports()):
            node = QtGui.QAction(port.device, self.ui.menuPort, checkable = True, statusTip = 'Connect via ' + port.device)
            node.triggered.connect(self.signalMapper.map)
            self.signalMapper.setMapping(node, port.device)
            group.addAction(node)
            self.ui.menuPort.addAction(node)
        self.tim.timeout.connect(self.readPort)
        self.get.timeout.connect(self.getData)

    def closeEvent(self, event):
        choice = QtGui.QMessageBox.question(self, 'Exit', 'Are you sure?', QtGui.QMessageBox.Yes| QtGui.QMessageBox.No)
        if choice == QtGui.QMessageBox.Yes:
            if self.ser.isOpen():
                self.ser.close()
            event.accept()
        else:
            event.ignore()

    def sendPacket(self, cmd, data = []):
        if self.ser.isOpen():
            packet = bytearray()
            crc = 0
            packet.append(0xDA)
            crc = crc + 0xDA
            packet.append(0x21)
            crc = crc + 0x21
            packet.append(0xA1)
            crc = crc + 0xA1
            packet.append(cmd)
            crc = crc + cmd
            if (len(data) > 0):
                packet.append(len(data))
                crc = crc + len(data)
                for i in range (0, len(data)):
                    if (data[i] == 0xDA) or (data[i] == 0xB0) or (data[i] == 0xBA):
                        packet.append(0xB0)
                        packet.append(0xFF - data[i])
                    else:
                        packet.append(data[i])
                    crc = crc + data[i]
            else:
                packet.append(0x00)
            if (crc == 0xDA) or (crc == 0xB0) or (crc == 0xBA):
                packet.append(0xB0)
                packet.append(0xFF - crc)
            else:
                packet.append(crc & 0xFF)
            packet.append(0xBA)
            print ' '.join('0x{:02X}'.format(x) for x in packet)
            self.ser.write(packet)
        else:
            self.ui.statusbar.showMessage('Port not open')

    def getData(self):
        self.sendPacket(0x03)
            
    def on_pushButtonOpenTurnOnFile_released(self):
        self.ui.statusbar.showMessage('Open Turn On File')

    def on_pushButtonOpenHumFile_released(self):
        self.ui.statusbar.showMessage('Open Hum File')

    def on_pushButtonOpenSwingFile_pressed(self):
        self.ui.statusbar.showMessage('Open Swing File')
        
    def on_pushButtonOpenHitFile_released(self):
        self.ui.statusbar.showMessage('Open Hit File')
        
    def on_pushButtonOpenClashFile_pressed(self):
        self.ui.statusbar.showMessage('Open Clash File')
            
    def on_pushButtonOpenTurnOffFile_released(self):
        self.ui.statusbar.showMessage('Open Turn Off File')

    def on_pushButtonLoad_released(self):
        self.ui.statusbar.showMessage('Load')

    @QtCore.pyqtSlot(bool)
    def on_pushButtonTest_clicked(self, arg):
        if arg:
            self.triggerTestStatus = 1
            self.get.start(self.getPeriodMs)
        else:
            self.triggerTestStatus = 0
            self.get.stop()

    @QtCore.pyqtSlot(bool)
    def on_actionOpen_triggered(self, arg):
        name = QtGui.QFileDialog.getOpenFileName(self)
        if QtCore.QFile.exists(name):
            with open(name, 'r') as file:
                text = file.read()
                self.ui.textEdit.setText(text)
                file.close()

    @QtCore.pyqtSlot(bool)
    def on_actionSaveAs_triggered(self, arg):
        name = QtGui.QFileDialog.getSaveFileName(self)
        if QtCore.QFile.exists(name):
            with open(name, 'w') as file:
                text = self.ui.textEdit.toPlainText()
                file.write(text)
                file.close()

    @QtCore.pyqtSlot(bool)
    def on_actionExit_triggered(self, arg):
        choice = QtGui.QMessageBox.question(self, 'Exit', 'Are you sure?', QtGui.QMessageBox.Yes | QtGui.QMessageBox.No)
        if choice == QtGui.QMessageBox.Yes:
            sys.exit()

    def setPort(self, name):
        self.ser = serial.Serial()
        self.ser.port = str(name)
        self.ser.baudrate = 57600
        self.ser.parity = serial.PARITY_NONE
        self.ser.stopbits = serial.STOPBITS_ONE
        self.ser.bytesize = serial.EIGHTBITS
        try: 
            self.ser.open()
        except Exception, e:
            self.ui.statusbar.showMessage('Port ' + name + ' not available')
        if self.ser.isOpen():
            self.tim.start(self.timPeriodMs)
            self.ui.statusbar.showMessage('Connected to ' + name)

    def readPort(self):
        data = bytearray()
        crc = 0
        if self.ser.isOpen() and (self.ser.inWaiting() >= 7):
            packet = map(ord, self.ser.read(7))
            print ' '.join('0x{:02X}'.format(x) for x in packet)
            for i in range (0, len(packet)):
                if (packet[i] == 0xB0):
                    packet[i] = 0xFF - packet[i + 1]
                    for j in range ((i + 1), (len(packet) - 1)):
                        packet[i] = packet[i + 1]
                    pop(packet)
            for i in range (0, (len(packet) - 2)):
                crc = crc + packet[i]
            if (crc & 0xFF) == packet[-2]:
                if (packet[0] == 0xDA) and (packet[1] == 0xA1) and (packet[2] == 0x21):
                    if packet[3] == 0x01:
                        self.ui.statusbar.showMessage('Acknowledged')
                    elif packet[3] == 0x02:
                        self.ui.statusbar.showMessage('Loaded')
                    elif packet[3] == 0x11:
                        for i in range (0, packet[4]):
                            data.append(packet[5 + i])
                        rotAngle = int(((data[0] & 0x7F) << 8) | data[1])
                        if (data[0] & 0x80) != 0:
                            rotAngle = -rotAngle
                        tiltAngle = int(((data[2] & 0x7F) << 8) | data[3])
                        if (data[2] & 0x80) != 0:
                            tiltAngle = -tiltAngle
                        self.ui.lineEditRealRot.setText(str(rotAngle))
                        self.ui.lineEditRealTilt.setText(str(tiltAngle))
                    elif packet[3] == 0x00:
                        self.ui.statusbar.showMessage('Error')
                    else:
                        self.ui.statusbar.showMessage('Unknown')

app = QtGui.QApplication(sys.argv)
main = MainWindow()
main.show()
sys.exit(app.exec_())
