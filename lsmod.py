import sys
import threading
import serial, time
import serial.tools.list_ports
import wave, struct
import numpy as np
from ui_lsmod import Ui_Lsmod
from PyQt4 import QtCore, QtGui
from PyQt4.phonon import Phonon
from PyQt4.QtCore import QObject, pyqtSignal, pyqtSlot

LSMOD_ADDR = 0x21
PC_ADDR    = 0xA1

MAX_TRACKS = 6

TOTAL_COLORS = 9
RGB_RED         = 0xFF0000
RGB_ORANGE_RED  = 0xFF4500
RGB_YELLOW      = 0xFFFF00
RGB_GREEN       = 0x00FF00
RGB_TURQUOISE   = 0x40E0D0
RGB_DODGER_BLUE = 0x1E90FF
RGB_BLUE_VIOLET = 0x8A2BE2
RGB_PURPLE      = 0x800080
RGB_ORCHID      = 0xDA70D6
Colors = [RGB_RED, RGB_ORANGE_RED, RGB_YELLOW, \
          RGB_GREEN, RGB_TURQUOISE, RGB_DODGER_BLUE, \
          RGB_BLUE_VIOLET, RGB_PURPLE, RGB_ORCHID]

LSMOD_BAUDRATE = 115200

LSMOD_PACKET_HDR = 0xDA
LSMOD_PACKET_MSK = 0xB0
LSMOD_PACKET_END = 0xBA

LSMOD_SRV_LEN      =   6
LSMOD_DATA_IDX_LEN =   4
LSMOD_DATA_MAX_LEN = 262
LSMOD_STAT_MAX_LEN =   6

LSMOD_CONTROL_PING       = 0x00
LSMOD_CONTROL_STAT       = 0x01
LSMOD_CONTROL_LOAD_BEGIN = 0x10
LSMOD_CONTROL_LOAD       = 0x11
LSMOD_CONTROL_LOAD_END   = 0x12

LSMOD_REPLY_ERROR  = 0x00
LSMOD_REPLY_ACK    = 0x01
LSMOD_REPLY_LOADED = 0x02
LSMOD_REPLY_STAT   = 0x03

class MainWindow(QtGui.QMainWindow):
    ui = Ui_Lsmod()
    ser = serial.Serial()
    tim = QtCore.QTimer()
    timPeriodMs = 10
    get = QtCore.QTimer()
    getPeriodMs = 100
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
    loadActivated = pyqtSignal()
    loadContinue = pyqtSignal()
    loadRepeat = QtCore.QTimer()
    loadRepeatPeriodMs = 100
    loadEnd = pyqtSignal()
    pushButtonColorsGroup = QtGui.QButtonGroup()
    pushButtonColors = {}
    
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
        self.ui.textEdit.setReadOnly(True)
        self.loadActivated.connect(self.loadSamples)
        self.loadContinue.connect(self.loadSamples)
        self.loadRepeat.timeout.connect(self.loadSamples)
        self.loadEnd.connect(self.endLoad)
        for i in range(int(np.sqrt(TOTAL_COLORS))):
            for j in range(int(np.sqrt(TOTAL_COLORS))):
                self.pushButtonColors[(i, j)] = QtGui.QPushButton()
                self.pushButtonColors[(i, j)].setCheckable(True)
                self.pushButtonColors[(i, j)].setStyleSheet( \
                    "QPushButton {\n" \
                    + "background-color: #%06x;" % Colors[i * int(np.sqrt(TOTAL_COLORS)) + j] \
                    + "border: 1px solid gray;" \
                    + "border-radius: 10px;" \
                    + "padding: 10px;" \
                    + "}" +
                    "QPushButton:focus {\n" \
                    + "border: none;" \
                    + "outline: none;" \
                    + "}" +
                    "QPushButton:checked {\n" \
                    + "border: 2px solid black;" \
                    + "}")
                self.pushButtonColorsGroup.addButton(self.pushButtonColors[(i, j)])
                self.ui.gridLayoutColors.addWidget(self.pushButtonColors[(i, j)], i, j)
        self.pushButtonColorsGroup.buttonClicked.connect(self.typeCode)

    def typeCode(self, button):
        color = button.palette().color(QtGui.QPalette.Background)
        self.ui.lineEditCode.setText("%02X%02X%02X" % (color.red(), color.green(), color.blue()))

    def closeEvent(self, event):
        choice = QtGui.QMessageBox.question(self, 'Exit', 'Are you sure?', QtGui.QMessageBox.Yes | QtGui.QMessageBox.No)
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
                for i in range (0, len(data)):
                    if (data[i] == LSMOD_PACKET_HDR) or (data[i] == LSMOD_PACKET_MSK) or (data[i] == LSMOD_PACKET_END):
                        packet.append(LSMOD_PACKET_MSK)
                        packet.append(0xFF - data[i])
                    else:
                        packet.append(data[i])
                    crc = crc + data[i]
            crc = crc & 0xFF
            if (crc == LSMOD_PACKET_HDR) or (crc == LSMOD_PACKET_MSK) or (crc == LSMOD_PACKET_END):
                packet.append(LSMOD_PACKET_MSK)
                packet.append(0xFF - crc)
            else:
                packet.append(crc)
            packet.append(LSMOD_PACKET_END)
            print ' '.join('0x{:02X}'.format(x) for x in packet)
            self.ser.write(packet)
        else:
            self.ui.textEdit.append('Port not open')

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
        self.trackIdx = 0
        self.pickFile()

    def on_pushButtonSet_released(self):
        code = self.ui.lineEditCode.text()
        if code:
            num = int('0x%s' % code, 16)
            self.ui.textEdit.append('Set color #%X' % num)

    def pickFile(self):
        if (self.trackIdx == 0) and QtCore.QFile.exists(self.turnOnFile):
            self.loadedFile = self.turnOnFile
            self.startLoad()
        elif (self.trackIdx == 1) and QtCore.QFile.exists(self.humFile):
            self.loadedFile = self.humFile
            self.startLoad()
        elif (self.trackIdx == 2) and QtCore.QFile.exists(self.swingFile):
            self.loadedFile = self.swingFile
            self.startLoad()
        elif (self.trackIdx == 3) and QtCore.QFile.exists(self.hitFile):
            self.loadedFile = self.hitFile
            self.startLoad()
        elif (self.trackIdx == 4) and QtCore.QFile.exists(self.clashFile):
            self.loadedFile = self.clashFile
            self.startLoad()
        elif (self.trackIdx == 5) and QtCore.QFile.exists(self.turnOffFile):
            self.loadedFile = self.turnOffFile
            self.startLoad()
        else:
            self.ui.textEdit.append('No file')

    def startLoad(self):
        self.ui.textEdit.append('Loading %s' % QtCore.QFileInfo(self.loadedFile).fileName())
        wav = wave.open(str(self.loadedFile), 'rb')
        (nchannels, sampwidth, framerate, nframes, comptype, compname) = wav.getparams()
        if framerate != 44100:
            self.ui.textEdit.append('Only 44100 framerate supported')
            return
        if comptype != 'NONE':
            self.ui.textEdit.append('Compressed file not supported yet')
            return
        frames = wav.readframes(nframes * nchannels)
        self.bytelist = []
        if sampwidth == 1:
            out = struct.unpack_from('%dB' %(nframes * nchannels), frames)
            if nchannels == 1:
                self.sound = np.array(out, dtype = np.uint8)
            elif nchannels == 2:
                self.ui.textEdit.append('Stereo sound detected - merging')
                left = np.array(out[0:][::2], dtype = np.uint8)
                right = np.array(out[1:][::2], dtype = np.uint8)
                self.sound = left / 2 + right / 2
            print ' '.join('{:d}'.format(x) for x in self.sound[0:50])
            self.values = self.sound
            print ' '.join('0x{:02X}'.format(x) for x in self.values[0:50])
            for elem in self.values:
                self.bytelist.append(elem & 0xFF)
            print ' '.join('0x{:02X}'.format(x) for x in self.bytelist[0:50])
        elif sampwidth == 2:
            out = struct.unpack_from('%dh' %(nframes * nchannels), frames)
            if nchannels == 1:
                self.sound = np.array(out, dtype = np.int32)
            elif nchannels == 2:
                self.ui.textEdit.append('Stereo sound detected - merging')
                left = np.array(out[0:][::2], dtype = np.int32)
                right = np.array(out[1:][::2], dtype = np.int32)
                self.sound = left / 2 + right / 2
            print ' '.join('{:d}'.format(x) for x in self.sound[0:50])
            self.values = (self.sound + 0x8000).astype(np.uint16)
            print ' '.join('0x{:04X}'.format(x) for x in self.values[0:50])
            for elem in self.values:
                self.bytelist.append((elem >> 8) & 0xFF)                 
            print ' '.join('0x{:02X}'.format(x) for x in self.bytelist[0:50])
        self.trackPos = 0
        self.sendPacket(LSMOD_CONTROL_LOAD_BEGIN, [self.trackIdx])

    def loadSamples(self):
        progressBarValue = self.ui.progressBar.maximum() * (self.trackIdx + float(self.trackPos + 1) / float(len(self.bytelist))) / MAX_TRACKS
        if progressBarValue > self.ui.progressBar.maximum():
            progressBarValue = self.ui.progressBar.maximum()
        progressBarFileValue = self.ui.progressBarFile.maximum() * float(self.trackPos + 1) / float(len(self.bytelist))
        if progressBarFileValue > self.ui.progressBarFile.maximum():
            progressBarFileValue = self.ui.progressBarFile.maximum()
        self.ui.progressBar.setValue(progressBarValue)
        self.ui.progressBarFile.setValue(progressBarFileValue)
        print self.trackPos
        print len(self.bytelist)
        if (self.trackPos < len(self.bytelist)):
            if (len(self.bytelist) - self.trackPos) > ((LSMOD_DATA_MAX_LEN - LSMOD_DATA_IDX_LEN) / 2):
                self.sendPacket(LSMOD_CONTROL_LOAD, [ord(i) for i in list(struct.pack('>I', self.trackPos))] + \
                                                    self.bytelist[self.trackPos:(self.trackPos + (LSMOD_DATA_MAX_LEN - LSMOD_DATA_IDX_LEN) / 2)])
            else:
                self.sendPacket(LSMOD_CONTROL_LOAD, [ord(i) for i in list(struct.pack('>I', self.trackPos))] + \
                                                    self.bytelist[self.trackPos:])
            self.loadRepeat.start(self.loadRepeatPeriodMs)
        else:
            self.sendPacket(LSMOD_CONTROL_LOAD_END, [self.trackIdx])

    def endLoad(self):
        self.ui.textEdit.append('Finished loading %s' % QtCore.QFileInfo(self.loadedFile).fileName())
        self.ui.progressBarFile.setValue(self.ui.progressBar.minimum())
        self.trackIdx = self.trackIdx + 1
        if self.trackIdx < MAX_TRACKS:
            self.pickFile()
        else:
            self.ui.textEdit.append('All files loaded')
            self.ui.progressBar.setValue(self.ui.progressBar.minimum())
       
    @QtCore.pyqtSlot(bool)
    def on_pushButtonTest_clicked(self, arg):
        if arg:
            self.triggerTestStatus = 1
            self.ui.pushButtonLoad.setEnabled(False)
            self.ui.pushButtonSet.setEnabled(False)
            self.get.start(self.getPeriodMs)
        else:
            self.triggerTestStatus = 0
            self.ui.pushButtonLoad.setEnabled(True)
            self.ui.pushButtonSet.setEnabled(True)
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
            self.ui.textEdit.append('Port ' + name + ' not available')
        if self.ser.isOpen():
            self.tim.start(self.timPeriodMs)
            self.ui.labelConnection.setText('Connected')
            font.setBold(True)
            self.ui.labelConnection.setFont(font)
            self.ui.pushButtonLoad.setEnabled(True)
            self.ui.pushButtonSet.setEnabled(True)
            self.ui.pushButtonTest.setEnabled(True)
            self.ui.textEdit.append('Connected to ' + name)

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
                            if packet[4] == LSMOD_CONTROL_LOAD_BEGIN:
                                self.loadActivated.emit()
                            elif packet[4] == LSMOD_CONTROL_LOAD_END:
                                self.loadEnd.emit()
                        elif packet[3] == LSMOD_REPLY_LOADED:
                            self.loadRepeat.stop()
                            self.trackPos = self.trackPos + packet[4]
                            self.loadContinue.emit()
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
                                if realX > self.ui.horizontalSliderX.maximum():
                                    self.ui.horizontalSliderX.setMaximum(realX)
                                if realX < self.ui.horizontalSliderX.minimum():
                                    self.ui.horizontalSliderX.setMinimum(realX)
                                if realY > self.ui.horizontalSliderY.maximum():
                                    self.ui.horizontalSliderY.setMaximum(realY)
                                if realY < self.ui.horizontalSliderY.minimum():
                                    self.ui.horizontalSliderY.setMinimum(realY)
                                if realZ > self.ui.horizontalSliderZ.maximum():
                                    self.ui.horizontalSliderZ.setMaximum(realZ)
                                if realZ < self.ui.horizontalSliderZ.minimum():
                                    self.ui.horizontalSliderZ.setMinimum(realZ)
                                self.ui.horizontalSliderX.setValue(realX)
                                self.ui.horizontalSliderY.setValue(realY)
                                self.ui.horizontalSliderZ.setValue(realZ)
                            else:
                                self.ui.textEdit.append('No data')
                        elif packet[3] == LSMOD_REPLY_ERROR:
                            self.loadRepeat.stop()
                            self.ui.textEdit.append('Error')
                        else:
                            self.ui.textEdit.append('Unknown')

app = QtGui.QApplication(sys.argv)
main = MainWindow()
main.show()
sys.exit(app.exec_())
