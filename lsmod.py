import sys
import threading
import serial, time
import serial.tools.list_ports
import wave, struct
import numpy as np
from ui_lsmod import Ui_Lsmod
from PyQt4 import QtCore, QtGui
from PyQt4.phonon import Phonon

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

class MainWindow(QtGui.QMainWindow):
    ui = Ui_Lsmod()
    ser = serial.Serial()
    tim = QtCore.QTimer()
    timPeriodMs = 100
    get = QtCore.QTimer()
    getPeriodMs = 250
    triggerTestStatus = 0
    turnOnFile = str()
    turnOn = Phonon.MediaObject()
    turnOnFilePlay = False
    humFile = str()
    hum = Phonon.MediaObject()
    humFilePlay = False
    swingFile = str()
    swing = Phonon.MediaObject()
    swingFilePlay = False
    hitFile = str()
    hit = Phonon.MediaObject()
    hitFilePlay = False
    clashFile = str()
    clash = Phonon.MediaObject()
    clashFilePlay = False
    turnOffFile = str()
    turnOff = Phonon.MediaObject()
    turnOffFilePlay = False
    
    def __init__(self):
        super(MainWindow, self).__init__()
        self.ui.setupUi(self)
        self.audioOutput = Phonon.AudioOutput(Phonon.MusicCategory, self)
        Phonon.createPath(self.turnOn, self.audioOutput)
        self.turnOn.stateChanged.connect(self.tunOnPlayStateChanged)
        Phonon.createPath(self.hum, self.audioOutput)
        self.hum.stateChanged.connect(self.humPlayStateChanged)
        Phonon.createPath(self.swing, self.audioOutput)
        self.swing.stateChanged.connect(self.swingPlayStateChanged)
        Phonon.createPath(self.hit, self.audioOutput)
        self.hit.stateChanged.connect(self.hitPlayStateChanged)
        Phonon.createPath(self.clash, self.audioOutput)
        self.clash.stateChanged.connect(self.clashPlayStateChanged)
        Phonon.createPath(self.turnOff, self.audioOutput)
        self.turnOff.stateChanged.connect(self.tunOffPlayStateChanged)
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
        self.get.timeout.connect(self.getStat)

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
            print ' '.join('0x{:02X}'.format(x) for x in packet)
            self.ser.write(packet)
        else:
            self.ui.statusbar.showMessage('Port not open')

    def getStat(self):
        self.sendPacket(LSMOD_CONTROL_STAT)
        
    def on_pushButtonOpenTurnOnFile_released(self):
        name = QtGui.QFileDialog.getOpenFileName(self, filter = "Wav files (*.wav)")
        if QtCore.QFile.exists(name):
            self.turnOnFile = name
            self.ui.lineEditTurnOnFile.setText(name)

    def on_pushButtonTurnOnFilePlay_released(self):
        if not self.turnOnFilePlay:
            self.ui.pushButtonTurnOnFilePlay.setText('||')
            self.turnOnFilePlay = True
            self.turnOn.setCurrentSource(Phonon.MediaSource(self.turnOnFile))
            self.turnOn.play()
        else:
            self.ui.pushButtonTurnOnFilePlay.setText('>')
            self.turnOnFilePlay = False
            self.turnOn.stop()

    def tunOnPlayStateChanged(self, newstate, oldstate):
        if newstate == Phonon.StoppedState or newstate == Phonon.PausedState:
            self.ui.pushButtonTurnOnFilePlay.setText('>')
            self.turnOnFilePlay = False

    def on_pushButtonOpenHumFile_released(self):
        name = QtGui.QFileDialog.getOpenFileName(self, filter = "Wav files (*.wav)")
        if QtCore.QFile.exists(name):
            self.humFile = name
            self.ui.lineEditHumFile.setText(name)

    def on_pushButtonHumFilePlay_released(self):
        if not self.humFilePlay:
            self.ui.pushButtonHumFilePlay.setText('||')
            self.humFilePlay = True
            self.hum.setCurrentSource(Phonon.MediaSource(self.humFile))
            self.hum.play()
        else:
            self.ui.pushButtonHumFilePlay.setText('>')
            self.humFilePlay = False
            self.hum.stop()

    def humPlayStateChanged(self, newstate, oldstate):
        if newstate == Phonon.StoppedState or newstate == Phonon.PausedState:
            self.ui.pushButtonHumFilePlay.setText('>')
            self.humFilePlay = False
            self.hum.stop()

    def on_pushButtonOpenSwingFile_released(self):
        name = QtGui.QFileDialog.getOpenFileName(self, filter = "Wav files (*.wav)")
        if QtCore.QFile.exists(name):
            self.swingFile = name
            self.ui.lineEditSwingFile.setText(name)

    def on_pushButtonSwingFilePlay_released(self):
        if not self.swingFilePlay:
            self.ui.pushButtonSwingFilePlay.setText('||')
            self.swingFilePlay = True
            self.swing.setCurrentSource(Phonon.MediaSource(self.swingFile))
            self.swing.play()
        else:
            self.ui.pushButtonSwingFilePlay.setText('>')
            self.swingFilePlay = False
            self.swing.stop()

    def swingPlayStateChanged(self, newstate, oldstate):
        if newstate == Phonon.StoppedState or newstate == Phonon.PausedState:
            self.ui.pushButtonSwingFilePlay.setText('>')
            self.swingFilePlay = False
            self.swing.stop()
        
    def on_pushButtonOpenHitFile_released(self):
        name = QtGui.QFileDialog.getOpenFileName(self, filter = "Wav files (*.wav)")
        if QtCore.QFile.exists(name):
            self.hitFile = name
            self.ui.lineEditHitFile.setText(name)

    def on_pushButtonHitFilePlay_released(self):
        if not self.hitFilePlay:
            self.ui.pushButtonHitFilePlay.setText('||')
            self.hitFilePlay = True
            self.hit.setCurrentSource(Phonon.MediaSource(self.hitFile))
            self.hit.play()
        else:
            self.ui.pushButtonHitFilePlay.setText('>')
            self.hitFilePlay = False
            self.hit.stop()

    def hitPlayStateChanged(self, newstate, oldstate):
        if newstate == Phonon.StoppedState or newstate == Phonon.PausedState:
            self.ui.pushButtonHitFilePlay.setText('>')
            self.hitFilePlay = False
            self.hit.stop()
        
    def on_pushButtonOpenClashFile_released(self):
        name = QtGui.QFileDialog.getOpenFileName(self, filter = "Wav files (*.wav)")
        if QtCore.QFile.exists(name):
            self.clashFile = name
            self.ui.lineEditClashFile.setText(name)

    def on_pushButtonClashFilePlay_released(self):
        if not self.clashFilePlay:
            self.ui.pushButtonClashFilePlay.setText('||')
            self.clashFilePlay = True
            self.clash.setCurrentSource(Phonon.MediaSource(self.clashFile))
            self.clash.play()
        else:
            self.ui.pushButtonClashFilePlay.setText('>')
            self.clashFilePlay = False
            self.clash.stop()

    def clashPlayStateChanged(self, newstate, oldstate):
        if newstate == Phonon.StoppedState or newstate == Phonon.PausedState:
            self.ui.pushButtonClashFilePlay.setText('>')
            self.clashFilePlay = False
            self.clash.stop()
            
    def on_pushButtonOpenTurnOffFile_released(self):
        name = QtGui.QFileDialog.getOpenFileName(self, filter = "Wav files (*.wav)")
        if QtCore.QFile.exists(name):
            self.turnOffFile = name
            self.ui.lineEditTurnOffFile.setText(name)

    def on_pushButtonTurnOffFilePlay_released(self):
        if not self.turnOffFilePlay:
            self.ui.pushButtonTurnOffFilePlay.setText('||')
            self.turnOffFilePlay = True
            self.turnOff.setCurrentSource(Phonon.MediaSource(self.turnOffFile))
            self.turnOff.play()
        else:
            self.ui.pushButtonTurnOffFilePlay.setText('>')
            self.turnOffFilePlay = False
            self.turnOff.stop()

    def tunOffPlayStateChanged(self, newstate, oldstate):
        if newstate == Phonon.StoppedState or newstate == Phonon.PausedState:
            self.ui.pushButtonTurnOffFilePlay.setText('>')
            self.turnOffFilePlay = False
            self.turnOff.stop()

    def on_pushButtonLoad_released(self):
        wav = wave.open(str(self.turnOnFile), 'rb')
        (nchannels, sampwidth, framerate, nframes, comptype, compname) = wav.getparams()
        if comptype == 'NONE':
            print wav.getparams()
            frames = wav.readframes(nframes * nchannels)
            out = struct.unpack_from('%dh' % nframes * nchannels, frames)
            if nchannels == 2:
                left = np.array(out[0:][::2], dtype = np.int16)
                right = np.array(out[1:][::2], dtype = np.int16)
            else:
                left = np.array(out, dtype = np.int16)
                right = left
            self.sendPacket(LSMOD_CONTROL_LOAD)
            #TODO: Send idex, framerate, total samples count
            #TODO: Send samples by block, each same size, WAV_BLOCK_SIZE
        else:
            self.ui.statusbar.showMessage('Compressed file not supported yet')
       
    @QtCore.pyqtSlot(bool)
    def on_pushButtonTest_clicked(self, arg):
        if arg:
            self.triggerTestStatus = 1
            self.ui.pushButtonLoad.setEnabled(False)
            self.get.start(self.getPeriodMs)
        else:
            self.triggerTestStatus = 0
            self.ui.pushButtonLoad.setEnabled(True)
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
        font = QtGui.QFont()
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
            self.ui.labelConnection.setText('Connected')
            font.setBold(True)
            self.ui.labelConnection.setFont(font)
            self.ui.pushButtonLoad.setEnabled(True)
            self.ui.pushButtonTest.setEnabled(True)
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
                print ' '.join('0x{:02X}'.format(x) for x in packet)
                endFound = False
                i = 0
                while not endFound and (i < len(packet)):
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
                        elif packet[3] == LSMOD_REPLY_STAT:
                            if len(packet) > 6:
                                for i in range (0, (len(packet) - 6)):
                                    data.append(packet[4 + i])
                                realX = int(((data[0] & 0x7F) << 8) | data[1])
                                if (data[0] & 0x80) != 0:
                                    realX = -realX
                                realY = int(((data[2] & 0x7F) << 8) | data[3])
                                if (data[2] & 0x80) != 0:
                                    realY = -realY
                                realZ = int(((data[4] & 0x7F) << 8) | data[5])
                                if (data[4] & 0x80) != 0:
                                    realZ = -realZ
                                self.ui.lineEditRealX.setText(str(realX))
                                self.ui.lineEditRealY.setText(str(realY))
                                self.ui.lineEditRealZ.setText(str(realZ))
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
