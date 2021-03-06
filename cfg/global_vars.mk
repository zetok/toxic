# Version
TOXIC_VERSION = 0.5.2
REV = $(shell git rev-list HEAD --count)
VERSION = $(TOXIC_VERSION)_r$(REV)

# Project directories
DOC_DIR = $(BASE_DIR)/doc
SRC_DIR = $(BASE_DIR)/src
SND_DIR = $(BASE_DIR)/sounds
MISC_DIR = $(BASE_DIR)/misc

# Project files
MANFILES = toxic.1 toxic.conf.5
DATAFILES = DHTnodes DNSservers toxic.conf.example
SNDFILES = ToxicContactOnline.wav ToxicContactOffline.wav ToxicError.wav
SNDFILES += ToxicRecvMessage.wav ToxicOutgoingCall.wav ToxicIncomingCall.wav
SNDFILES += ToxicTransferComplete.wav ToxicTransferStart.wav

# Install directories
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
DATADIR = $(PREFIX)/share/toxic
MANDIR = $(PREFIX)/share/man
