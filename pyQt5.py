#! python3
import sys
import win32api
from PyQt5.QtWidgets import QApplication, QWidget, \
    QToolTip, QPushButton, QMessageBox, QDesktopWidget, \
    QMainWindow, QAction, QMenu, QStatusBar
from PyQt5.QtGui import QIcon, QFont
from PyQt5.QtCore import QCoreApplication

'''
#面向过程
app = QApplication(sys.argv)
w = QWidget()
w.resize(300, 150)
w.setFixedSize(700, 400)
x, y = win32api.GetSystemMetrics(0), win32api.GetSystemMetrics(1)
w.move((x - 250) / 2, (y - 150) / 2)
w.setWindowTitle('将图片转换为jpg 1920*1080')
w.show()
sys.exit(app.exec_())
'''
x , y= 0,0
class Example(QMainWindow):
    def __init__(self):
        super().__init__()#调用父类构造函数
        self.initUI()

    def initUI(self):

        global x,y

        QToolTip.setFont(QFont('SansSerif', 10))
        self.setToolTip('This is a <b>QWidget</b> widget')

        btn = QPushButton('Quit', self)
        btn.setToolTip('This is a <b>QPushButton</b> widget')
        # btn.clicked.connect(QCoreApplication.instance().quit) #clicked btn and exit app
        btn.clicked.connect(QApplication.instance().exit) #clicked btn and exit app
        btn.resize(btn.sizeHint())
        btn.move(50, 50)

        exitAct:QAction = QAction(QIcon('46.jpg'), 'Exit', self)
        exitAct.setShortcut('Ctrl+Q')
        exitAct.setStatusTip('Exit application')
        exitAct.triggered.connect(QApplication.instance().quit)

        menuBar = self.menuBar()
        fileMenu = menuBar.addMenu('File')
        fileMenu.addAction(exitAct)

        self.toolbar = self.addToolBar('Exit')
        self.toolbar.addAction(exitAct)

        # subMenu add to FileMenu
        subMenu = QMenu('Import', self)
        subAct = QAction('Import file', self)
        subMenu.addAction(subAct)
        fileMenu.addMenu(subMenu)

        #checkMenu
        viewSBar:QAction = QAction('View statusbar', self, checkable=True)
        viewSBar.setStatusTip('View statusbar')
        viewSBar.setChecked(True)
        viewSBar.triggered.connect(self.toggleMenu)
        fileMenu.addAction(viewSBar)

        # x, y = win32api.GetSystemMetrics(0), win32api.GetSystemMetrics(1)
        # self.setGeometry((x-700)/2, (y-400)/2, 700, 400)
        self.center()
        self.setFixedSize(700, 400)
        self.setWindowTitle('将图片转换成jpg 1080')
        self.statusbar = self.statusBar()
        self.statusbar.showMessage('Ready')
        self.setWindowIcon(QIcon('46.jpg'))
        self.show()

    def center(self):
        qr = self.frameGeometry()#get self RECT
        cp = QDesktopWidget().availableGeometry().center()#
        qr.moveCenter(cp)
        self.move(qr.topLeft())

    #When Widgets is closing, call this function by automaticlly
    #virtualFunction
    def closeEvent(self, event):
        reply = QMessageBox.question(self, 'Message', 'Are you sure to quit',
                                     QMessageBox.Yes|QMessageBox.No, QMessageBox.No)
        if reply == QMessageBox.Yes:
            event.accept()
        else:
            event.ignore()

    #virtual Function
    def contextMenuEvent(self, event):
        cmenu = QMenu(self)

        newAct = cmenu.addAction('New')
        opnAct = cmenu.addAction('Open')
        quitAct = cmenu.addAction('Quit')

        #which Action
        print(type(event), event.pos())
        action = cmenu.exec_(self.mapToGlobal(event.pos()))

        if action == quitAct:
            QApplication.instance().quit()

    def toggleMenu(self, state):
        print(state)
        if state:
            self.statusbar.show()
        else:
            self.statusbar.hide()

if __name__ == '__main__':
    app = QApplication(sys.argv)
    ex = Example()
    sys.exit(app.exec_())
