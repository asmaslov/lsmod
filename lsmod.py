import sys
import threading
import serial, time
import serial.tools.list_ports
from ui_lsmod import Ui_Lsmod
from PyQt4 import QtCore, QtGui

LSMOD_ADDR = 0x21
PC_ADDR    = 0xA1

LSMOD_BAUDRATE = 57600

LSMOD_PACKET_HDR = 0xDA
LSMOD_PACKET_MSK = 0xB0
LSMOD_PACKET_END = 0xBA

LSMOD_CONTROL_PING = 0x00
LSMOD_CONTROL_STAT = 0x01
LSMOD_CONTROL_LOAD = 0x02
LSMOD_CONTROL_READ = 0x03

LSMOD_REPLY_ERROR  = 0x00
LSMOD_REPLY_ACK    = 0x01
LSMOD_REPLY_LOADED = 0x02
LSMOD_REPLY_STAT   = 0x10
LSMOD_REPLY_DATA   = 0x11

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
    getPeriodMs = 250
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
            packet.append(LSMOD_PACKET_HDR)
            crc = crc + LSMOD_PACKET_HDR
            packet.append(LSMOD_ADDR)
            crc = crc + LSMOD_ADDR
            packet.append(PC_ADDR)
            crc = crc + PC_ADDR
            packet.append(cmd)
            crc = crc + cmd
            if (len(data) > 0):
                packet.append(len(data))
                crc = crc + len(data)
                for i in range (0, len(data)):
                    if (data[i] == LSMOD_PACKET_HDR) or (data[i] == LSMOD_PACKET_MSK) or (data[i] == LSMOD_PACKET_END):
                        packet.append(LSMOD_PACKET_MSK)
                        packet.append(0xFF - data[i])
                    else:
                        packet.append(data[i])
                    crc = crc + data[i]
            if (crc == LSMOD_PACKET_HDR) or (crc == LSMOD_PACKET_MSK) or (crc == LSMOD_PACKET_END):
                packet.append(LSMOD_PACKET_MSK)
                packet.append(0xFF - crc)
            else:
                packet.append(crc & 0xFF)
            packet.append(LSMOD_PACKET_END)
            #print ' '.join('0x{:02X}'.format(x) for x in packet)
            self.ser.write(packet)
        else:
            self.ui.statusbar.showMessage('Port not open')

    def getData(self):
        self.sendPacket(LSMOD_CONTROL_READ)
            
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
        self.ser.baudrate = LSMOD_BAUDRATE
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
        packet = list()
        crc = 0
        endFound = False
        if self.ser.isOpen():
            while self.ser.inWaiting() and not endFound:
                packet.append(ord(self.ser.read()))
                if packet[-1] == LSMOD_PACKET_END:
                    endFound = True
            if len(packet) >= 6:
                #print ' '.join('0x{:02X}'.format(x) for x in packet)
                endFound = False
                i = 0
                while not endFound:
                    if (packet[i] == LSMOD_PACKET_END):
                        endFound = True
                    elif (packet[i] == LSMOD_PACKET_MSK):
                        packet[i] = 0xFF - packet[i + 1]
                        for j in range ((i + 1), (len(packet) - 1)):
                            packet[j] = packet[j + 1]
                        packet.pop()
                    i = i + 1
                for i in range (0, (len(packet) - 2)):
                    crc = crc + packet[i]
                if (crc & 0xFF) == packet[-2]:
                    if (packet[0] == LSMOD_PACKET_HDR) and (packet[1] == PC_ADDR) and (packet[2] == LSMOD_ADDR):
                        if packet[3] == LSMOD_REPLY_ACK:
                            self.ui.statusbar.showMessage('Acknowledged')
                        elif packet[3] == LSMOD_REPLY_LOADED:
                            self.ui.statusbar.showMessage('Loaded')
                        elif packet[3] == LSMOD_REPLY_DATA:
                            if len(packet) > 6:
                                for i in range (0, (len(packet) - 6)):
                                    data.append(packet[4 + i])
                                rotAngle = int(((data[0] & 0x7F) << 8) | data[1])
                                if (data[0] & 0x80) != 0:
                                    rotAngle = -rotAngle
                                tiltAngle = int(((data[2] & 0x7F) << 8) | data[3])
                                if (data[2] & 0x80) != 0:
                                    tiltAngle = -tiltAngle
                                self.ui.lineEditRealRot.setText(str(rotAngle))
                                self.ui.lineEditRealTilt.setText(str(tiltAngle))
                            else:
                                self.ui.statusbar.showMessage('No data')
                        elif packet[3] == LSMOD_REPLY_ERROR:
                            self.ui.statusbar.showMessage('Error')
                        else:
                            self.ui.statusbar.showMessage('Unknown')

app = QtGui.QApplication(sys.argv)
main = MainWindow()
main.show()
sys.exit(app.exec_())
