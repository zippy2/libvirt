#!/usr/bin/env python

from lxml import etree
import sys

tree = etree.parse(sys.stdin)

root = tree.getroot()

def reverse(node):
    newe = []
    for e in list(node):
        node.remove(e)
        newe.append(e)
    newe.reverse()
    for e in newe:
        node.append(e)
        pass

    for e in newe:
        reverse(e)

reverse(root)
print(etree.tostring(tree, pretty_print=True).decode('utf8'))


#$ for i in `find -name '*.xml' `
#  do
#    echo $1
#    ./xmlrngfuzz.py < $i >  ${i%%xml}rev.xml
#  done
