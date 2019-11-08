"""Summary files

Examples
--------
Load and read a case:

>>> case = ecl3.summary.load('CASE.SMSPEC')
>>> case.nlist
123
>>> case.keywords[:3]
['TIME', 'YEARS', 'FOPR']
>>> report = case.readall('CASE.UNSMRY')
"""
import logging
logging.getLogger(__name__)

from .specification import summary
from .specification import load

__all__ = [
    'load',
    'summary',
]
