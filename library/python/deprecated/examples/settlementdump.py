import sys
from cStringIO import StringIO
import pydfapi

df = pydfapi.API("Memory.xml")

def print_settlement(settlement, english_words, foreign_words):
    s = StringIO()
