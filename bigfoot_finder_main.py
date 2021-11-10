# Program: Bigfoot Finder tool
# Class: CS361 Fall 2021
# Author: Megan Janitor

import webbrowser
import requests
from PyQt5 import QtCore
from PyQt5 import QtGui
from PyQt5.QtCore import Qt
from PyQt5.QtGui import *
from PyQt5.QtWidgets import *


def create_GUI():
    # Creates a PyQt5 application
    app = QApplication([])
    app.setStyle("Fusion")
    dark_palette = QPalette()
    color = QColor
    qt = Qt

    # ----Gives a "dark mode" vibe----#
    dark_palette.setColor(QPalette.Window, QColor(53, 53, 53))
    dark_palette.setColor(QPalette.WindowText, Qt.white)
    dark_palette.setColor(QPalette.Base, QColor(25, 25, 25))
    dark_palette.setColor(QPalette.AlternateBase, QColor(53, 53, 53))
    dark_palette.setColor(QPalette.ToolTipBase, Qt.white)
    dark_palette.setColor(QPalette.ToolTipText, Qt.white)
    dark_palette.setColor(QPalette.Text, Qt.white)
    dark_palette.setColor(QPalette.Button, QColor(53, 53, 53))
    dark_palette.setColor(QPalette.ButtonText, Qt.white)
    dark_palette.setColor(QPalette.BrightText, Qt.red)
    dark_palette.setColor(QPalette.Link, QColor(42, 130, 218))
    dark_palette.setColor(QPalette.Highlight, QColor(42, 130, 218))
    dark_palette.setColor(QPalette.HighlightedText, Qt.black)

    app.setPalette(dark_palette)

    app.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }")

    # ----Creates a basic window widget to hold more widgets----#
    window = QWidget()
    window.setFixedHeight(600)
    window.setFixedWidth(1000)
    window.setWindowTitle("Bigfoot Tracker 1.0")

    # ----Creates a grid layout and adds widgets to the grid----#
    grid_layout = QGridLayout()
    window.setLayout(grid_layout)

    # ----Set elements of grid----#

    # ----Recent News Label Widget----#
    twitterLabel = QLabel("Recent News")
    twitterLabel.setAlignment(QtCore.Qt.AlignCenter)
    twitterLabel.setFont(QtGui.QFont("Times", 14, QtGui.QFont.Bold))
    grid_layout.addWidget(twitterLabel, 1, 0)

    # ----Page header label Widget----#
    headerLabel = QLabel("Bigfoot Tracker! Your go-to Bigfoot finding tool")
    headerLabel.setAlignment(QtCore.Qt.AlignCenter)
    headerLabel.setFont(QtGui.QFont("Times", 14, QtGui.QFont.Bold))
    grid_layout.addWidget(headerLabel, 0, 1)

    # ----Function to close the program for close button----#
    def closeButtonClick(window):
        exit()

    # ---Function to add display sighting data widget on search button click----#
    def displaySightingData():

        sightingData = {}
        counties = []
        dates = []
        descriptions = []

        # Format request url
        url_prefix = "http://bfro-scraper.herokuapp.com/?state="
        state_selected = comboBox.currentText()
        url_postfix = "&numSightings=20"
        url_prefix += state_selected
        url_prefix += url_postfix

        response = requests.get(url_prefix)
        json_response = response.json()
        unrefined_data = json_response.get('data')
        for i in unrefined_data:
            counties.append(i["county"])
            dates.append(i["date"])
            descriptions.append(i["description"])


        sightingData['county'] = counties
        sightingData['date'] = dates
        sightingData['description'] = descriptions


        # ---Sets table data----#
        class TableView(QTableWidget):
            def __init__(self, data, *args):
                QTableWidget.__init__(self, *args)
                self.data = data
                self.setData()
                self.resizeColumnsToContents()
                self.resizeRowsToContents()

            def setData(self):
                horHeaders = []
                for n, key in enumerate(sorted(self.data.keys())):
                    horHeaders.append(key)
                    for m, item in enumerate(self.data[key]):
                        newitem = QTableWidgetItem(item)
                        self.setItem(m, n, newitem)
                self.setHorizontalHeaderLabels(horHeaders)
                self.horizontalHeader().setStretchLastSection(True)
                self.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)

        # Here we set table data based on which state was selected
        dataTable = TableView(sightingData, 20, 3)
        grid_layout.addWidget(dataTable, 2, 1)
        window.show()

    # ----Close button widget----#
    closeButton = QPushButton("Close")
    grid_layout.addWidget(closeButton, 1, 3)
    closeButton.clicked.connect(closeButtonClick)

    # ----Twitter data widget----#
    twitterData = QGroupBox("From Twitter:")
    twitterData.setAlignment(QtCore.Qt.AlignCenter)
    grid_layout.addWidget(twitterData, 2, 0)
    news_box = QVBoxLayout()
    twitterData.setLayout(news_box)

    # ----Tweets to go inside the twutterData group box----#
    news_one = QLabel(
        "BFRO UPDATES @BFRO_Updates--10/13\nJust posted on BFRO web site:\nSept 22, 2021 ; Texas (Marion County)\n"
        "Northeast corner of the state, near Lousiana\n border. Class B sighting by motorist in morning \ndaylight.\nFor details and Google Maps pin see report here:\n"
        "http://bfro.net/GDB/show_report.asp?id=70712\nDiscussion: https://bit.ly/3mN8F9l")
    news_one.setAlignment(QtCore.Qt.AlignCenter)
    news_box.addWidget(news_one)
    news_two = QLabel(
        "BFRO UPDATES @BFRO_Updates--10/9\nJust posted: March '21, Class A\n daylight sighting from eastern South Carolina.\n"
        "Witness: A long haul trucker who was delivering\n a load to a nearby steel plant, driving near \nthe Cooper River (Francis Marion National Forest)\n"
        "which was flooded at the time. \nSee https://bit.ly/3FyoI3C")
    news_two.setAlignment(QtCore.Qt.AlignCenter)
    news_box.addWidget(news_two)
    news_three = QLabel(
        "BFRO UPDATES @BFRO_Updates--10/13\nJust posted on BFRO web site:\nSept 22, 2021 ; Texas (Marion County)\n"
        "Northeast corner of the state, near Lousiana\n border. Class B sighting by motorist in morning \ndaylight.\nFor details and Google Maps pin see report here:\n"
        "http://bfro.net/GDB/show_report.asp?id=70712\nDiscussion: https://bit.ly/3mN8F9l")
    news_three.setAlignment(QtCore.Qt.AlignCenter)
    news_box.addWidget(news_three)

    # ---Display sighting data button Widget----#
    comboBox = QComboBox()
    comboBox.addItem("Display historical sighting data for: ")
    states = ["Alaska", "Alabama", "Arkansas", "Arizona", "California", "Colorado", "Connecticut",
              "District of Columbia",
              "Delaware", "Florida", "Georgia", "Guam", "Hawaii", "Iowa", "Idaho", "Illinois", "Indiana", "Kansas",
              "Kentucky", "Louisiana", "Massachusetts", "Maryland", "Maine", "Michigan", "Minnesota", "Missouri",
              "Mississippi", "Montana", "North Carolina", "North Dakota", "Nebraska", "New Hampshire", "New Jersey",
              "New Mexico", "Nevada", "New York", "Ohio", "Oklahoma", "Oregon", "Pennsylvania", "Puerto Rico",
              "Rhode Island", "South Carolina", "South Dakota", "Tennessee", "Texas", "Utah", "Virginia", "Vermont",
              "Washington", "Wisconsin", "West Virginia", "Wyoming"]

    for s in states:
        comboBox.addItem(s)
    grid_layout.addWidget(comboBox, 1, 1)
    comboBox.activated[str].connect(displaySightingData)
    # searchLabel.clicked.connect(displaySightingData)

    # ----Bigfoot image Widget----#
    bigfootImage = QLabel("Bigfoot")
    bigfootImage.setAlignment(QtCore.Qt.AlignCenter)
    bigfootImage.setPixmap(QtGui.QPixmap('spookyedit.jpeg'))
    grid_layout.addWidget(bigfootImage, 2, 3)

    # ----Function to open browser for wiki page----#
    # --NOTE: Currently opens page in browser for MVP, make this open in PyQt5 window later--#
    def openBrowser():
        webbrowser.open('https://en.wikipedia.org/wiki/Bigfoot')

    # ----Wiki display button Widget----#
    wiki = QPushButton("What is Bigfoot?")
    wiki.setFont(QtGui.QFont("Times", 14, QtGui.QFont.Bold))
    grid_layout.addWidget(wiki, 3, 0)
    wiki.clicked.connect(openBrowser)

    # Sets the layout now that all widgets are added
    window.setLayout(grid_layout)
    window.setGeometry(300, 300, 200, 200)

    # Must use this to actually display the window
    window.show()
    # Runs the app until the user closes it
    app.exec()


create_GUI()
