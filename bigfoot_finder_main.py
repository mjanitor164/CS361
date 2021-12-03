# Program: Bigfoot Finder tool
# Class: CS361 Fall 2021
# Author: Megan Janitor

import webbrowser
import requests
import tweepy
from PyQt5 import QtCore
from PyQt5 import QtGui
from PyQt5.QtCore import Qt
from PyQt5.QtGui import *
from PyQt5.QtWidgets import *

STATES = ["Alaska", "Alabama", "Arkansas", "Arizona", "California", "Colorado", "Connecticut",
          "District of Columbia",
          "Delaware", "Florida", "Georgia", "Guam", "Hawaii", "Iowa", "Idaho", "Illinois", "Indiana", "Kansas",
          "Kentucky", "Louisiana", "Massachusetts", "Maryland", "Maine", "Michigan", "Minnesota", "Missouri",
          "Mississippi", "Montana", "North Carolina", "North Dakota", "Nebraska", "New Hampshire", "New Jersey",
          "New Mexico", "Nevada", "New York", "Ohio", "Oklahoma", "Oregon", "Pennsylvania", "Puerto Rico",
          "Rhode Island", "South Carolina", "South Dakota", "Tennessee", "Texas", "Utah", "Virginia", "Vermont",
          "Washington", "Wisconsin", "West Virginia", "Wyoming"]


# Takes the application as a param and sets GUI coloring to create a "dark mode" theme
def set_dark_mode(app):
    dark_palette = QPalette()

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


# Creates a button that closes the application when clicked
def create_close_button(grid_layout):
    # ----Function to close the program for close button----#
    def closeButtonClick(window):
        exit()

    closeButton = QPushButton("Close")
    closeButton.setFont(QFont('Arial', 14))
    grid_layout.addWidget(closeButton, 1, 3)
    closeButton.clicked.connect(closeButtonClick)


def create_twitter_widget(grid_layout):
    # To do---set header of each tweet to twitter format as defined:
    #  "BFRO Updates @BFRO_Updates - Nov 23"
    twitterData = QGroupBox("@BFRO_Updates")
    twitterData.setFont(QFont('Helvetica Neue', 12))
    twitterData.setAlignment(QtCore.Qt.AlignCenter)
    grid_layout.addWidget(twitterData, 2, 0)
    news_box = QVBoxLayout()
    twitterData.setLayout(news_box)
    # setting background color for news_box
    twitter_palette = QPalette()
    twitter_palette.setColor(QPalette.Window, QColor("black"))
    twitterData.setAutoFillBackground(True)
    twitterData.setPalette(twitter_palette)

    # Scrapes twitter for latest tweets from @BFRO_Updates
    scrape_twitter(news_box)


def scrape_twitter(news_box):
    # Key hidden for GitHub release
    b_token = ""

    client = tweepy.Client(bearer_token=b_token)

    # User id for the twitter user whose tweets we are scraping
    user = 873624260

    # This gets the tweets from the user with the specified user ID
    tweets = client.get_users_tweets(id=user, tweet_fields=['context_annotations', 'created_at', 'geo'])

    # Data structure for holding tweets
    individual_tweets = []
    for i in tweets.data:
        individual_tweets.append(i.text)

    # Create each of the tweet widgets
    news_one = QLabel(individual_tweets[1])
    news_one.setFont(QFont('Helvetica Neue', 11))
    news_one.setWordWrap(True)
    news_one.setAlignment(QtCore.Qt.AlignCenter)
    news_box.addWidget(news_one)
    news_two = QLabel(individual_tweets[2])
    news_two.setFont(QFont('Helvetica Neue', 11))
    news_two.setWordWrap(True)
    news_two.setAlignment(QtCore.Qt.AlignCenter)
    news_box.addWidget(news_two)
    news_three = QLabel(individual_tweets[3])
    news_three.setFont(QFont('Helvetica Neue', 11))
    news_three.setWordWrap(True)
    news_three.setAlignment(QtCore.Qt.AlignCenter)
    news_box.addWidget(news_three)

    # Setting background color for individual tweet widgets
    twitter_palette = QPalette()
    twitter_palette.setColor(QPalette.Window, QColor("black"))
    news_one.setAutoFillBackground(True)
    news_two.setAutoFillBackground(True)
    news_three.setAutoFillBackground(True)
    news_one.setPalette(twitter_palette)
    news_two.setPalette(twitter_palette)
    news_three.setPalette(twitter_palette)


# Function to display images for the GUI. Currently only has one image
def displayImages(grid_layout):
    bigfootImage = QLabel("Bigfoot")
    bigfootImage.setAlignment(QtCore.Qt.AlignCenter)
    bigfootImage.setPixmap(QtGui.QPixmap('spookyedit.jpeg'))
    grid_layout.addWidget(bigfootImage, 2, 3)


# Function to open browser window with wikipedia's bigfoot page
def display_wiki(grid_layout):
    # Function that opens the wiki bigfoot page
    def openWiki():
        webbrowser.open('https://en.wikipedia.org/wiki/Bigfoot')

    wiki = QPushButton("What is Bigfoot?")
    wiki.setFont(QtGui.QFont("Times", 14, QtGui.QFont.Bold))
    grid_layout.addWidget(wiki, 3, 0)
    wiki.clicked.connect(openWiki)


# Creates the GUI by creating the window, formatting, and all of the widgets (elements) of the program.
def create_GUI():
    # Creates a PyQt5 application
    app = QApplication([])
    app.setStyle("Fusion")
    color = QColor
    qt = Qt
    comboBox = QComboBox()

    # ----Gives a "dark mode" vibe----#
    set_dark_mode(app)

    # ----Creates a basic window widget to hold more widgets----#
    window = QWidget()
    window.setFixedHeight(600)
    window.setFixedWidth(1000)
    window.setWindowTitle("Bigfoot Tracker 1.0")

    # ----Creates a grid layout and adds widgets to the grid----#
    grid_layout = QGridLayout()
    window.setLayout(grid_layout)

    # ----Recent News Label Widget----#
    twitterLabel = QLabel("This just in! New sightings: ")
    twitterLabel.setFont(QFont('Arial', 12))
    twitterLabel.setAlignment(QtCore.Qt.AlignCenter)
    twitterLabel.setFont(QtGui.QFont("Times", 14, QtGui.QFont.Bold))
    grid_layout.addWidget(twitterLabel, 1, 0)

    # ----Page header label Widget----#
    headerLabel = QLabel("Bigfoot Tracker! A Bigfoot finding tool")
    headerLabel.setAlignment(QtCore.Qt.AlignCenter)
    headerLabel.setFont(QtGui.QFont("Times", 18, QtGui.QFont.Bold))
    grid_layout.addWidget(headerLabel, 0, 1)

    # ----Close button widget----#
    create_close_button(grid_layout)

    # ----Twitter tweet news widget----#
    create_twitter_widget(grid_layout)

    # ---Uses teammate's service to scrape historical sighting data---#
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

    # Creates a scrollbox for the user to select a state to display data for
    comboBox.addItem("Display historical sighting data for:")
    comboBox.setFont(QFont('Arial', 14))

    for s in STATES:
        comboBox.addItem(s)
    grid_layout.addWidget(comboBox, 1, 1)
    comboBox.activated[str].connect(displaySightingData)

    # ----Bigfoot image Widget----#
    displayImages(grid_layout)

    # ----Function that creates the display wikipedia button and adds--------#
    # ----the functionality to make it open a browser window when clicked----#
    display_wiki(grid_layout)

    # Sets the layout now that all widgets are added
    window.setLayout(grid_layout)
    window.setGeometry(300, 300, 200, 200)

    window.show()
    # Runs the app until the user closes it
    app.exec()


def main():
    create_GUI()


if __name__ == "__main__":
    main()
