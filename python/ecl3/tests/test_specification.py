import hypothesis
from hypothesis import given
from hypothesis import strategies as st
import pytest
import datetime
import numpy as np

from .. import summary
from .. import core
from . import data

minimal_keywords = {
    'DIMENS':   [2, 1, 1, 1, 0, 0],
    'KEYWORDS': ['WOPR', 'WOPT'],
    'WGNAMES':  ['W1', 'W2'],
    'UNITS':    ['SM3/DAY', 'SM3'],
    'STARTDAT': [5, 3, 1971, 9, 37, 14917],
    'NUMS':     [1, 1],
    'MEASRMNT': ['O:FLOWRA', 'TE      ', 'O:FLOWVO', 'LUME    '],
}

optional_keywords = {
    'LGRS':     ['LGR1    ', 'LGR2    '],
    'NUMLX':    [1, 7],
    'NUMLY':    [2, 11],
    'NUMLZ':    [3, 13],
    'LENGTHS':  [1.2, 2.9],
    'LENUNITS': ['M'],
    'LGRNAMES': ['LGRID   '],
    'LGRVEC':   [2],
    'LGRTIMES': [2],
    'RUNTIMEI': [
        2, # finished
        0, # initial report number
        20, # current report number
        2017, 2, 13, 15, 44, 42, # intial report date
        2017, 2, 13, 15, 45, 11, # current report date
        30, 6, 2018, 1, 53, 9, 40, 19, 0, 0, 59764, 2,
        1, 1, 0, 0, 0, 0, 0,
        2, # assigned to BASIC
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ],
    'RUNTIMED': [545.0, 29.3, 32.2, 0.0, 0.0],
    'STEPRESN': ['REASON  '] + ['        '] * 29,
    'XCOORD': [2.1, 9.3],
    'YCOORD': [8.2, 0.0],
    'TIMESTMP': [1997, 3, 21, 15, 54, 30],
}

def makedate(dt):
    """
    The STARTDAT date format does not use seconds, but instead embed that
    information in microseconds, but python datetime does not support second
    wrap-around generate a STARTDAT array from a datetime.
    """
    day = dt.day
    month = dt.month
    year = dt.year
    hour = dt.hour
    minute = dt.minute
    microsecond = dt.second * 1000000 + dt.microsecond
    return dt, [day, month, year, hour, minute, microsecond]

@given(st.builds(makedate, st.datetimes()))
def test_valid_startdates(startdates):
    expected, array = startdates
    kw = ('STARTDAT', array)
    s = summary.summary([kw])
    assert s.startdate == expected

def test_minimal_from_keywords():
    s = summary.summary(minimal_keywords)
    s.check_integrity()
    assert s.nlist == 2
    assert s.keywords == ['WOPR', 'WOPT']
    assert s.wgnames == ['W1', 'W2']
    assert s.units == ['SM3/DAY', 'SM3']
    assert s.startdate == datetime.datetime(
            day = 5,
            month = 3,
            year = 1971,
            hour = 9,
            minute = 37,
            microsecond = 14917,
        )
    assert s.nums == [1, 1]
    assert s.measurements == ['O:FLOWRATE', 'O:FLOWVOLUME']

def test_with_optional_from_keywords():
    kws = dict(minimal_keywords)
    kws.update(optional_keywords)
    s = summary.summary(kws)
    s.check_integrity()

    assert s.lgrs == ['LGR1', 'LGR2']
    assert s.numlx == [1, 7]
    assert s.numly == [2, 11]
    assert s.numlz == [3, 13]
    assert s.lengths == [1.2, 2.9]
    assert s.lenunits == 'M'
    assert s.lgrnames == ['LGRID']
    assert s.lgrvec == [2]
    assert s.lgrtimes == [2]

    monitor = s.runtime_monitor
    assert monitor.finished
    assert monitor.initial_reportno == 0
    assert monitor.current_reportno == 20
    assert monitor.initial_timestamp == datetime.datetime(
        year = 2017,
        month = 2,
        day = 13,
        hour = 15,
        minute = 44,
        second = 42,
    )
    assert monitor.current_timestamp == datetime.datetime(
        year = 2017,
        month = 2,
        day = 13,
        hour = 15,
        minute = 45,
        second = 11,
    )
    assert monitor.basic == 2
    assert monitor.double == [545.0, 29.3, 32.2, 0.0, 0.0]

    assert s.step_reason == 'REASON'
    assert s.xcoord == [2.1, 9.3]
    assert s.ycoord == [8.2, 0.0]
    assert s.timestamp == datetime.datetime(
        year = 1997,
        month = 3,
        day = 21,
        hour = 15,
        minute = 54,
        second = 30,
    )

def test_intehead_mappings():
    systems = [
        (1, 'METRIC'),
        (2, 'FIELD'),
        (3, 'LAB'),
        (4, 'PVT-M'),
    ]

    simulators = [
        (100, 'ECLIPSE 100'),
        (300, 'ECLIPSE 300'),
        (500, 'ECLIPSE 300 (thermal option)'),
        (700, 'INTERSECT'),
        (800, 'FrontSim'),
    ]

    kws = dict(minimal_keywords)
    for n, name in systems:
        for s, sim in simulators:
            kws['INTEHEAD'] = [n, s]
            s = summary.summary(kws)
            assert s.unitsystem == name
            assert s.simulator == sim

def test_intehead_unitsystem_miss_warns(caplog):
    invalid = 0
    _ = summary.summary([('INTEHEAD', [invalid, 100])])
    assert len(caplog.records) == 1

def test_intehead_simulator_miss_warns(caplog):
    invalid = 10
    _ = summary.summary([('INTEHEAD', [1, invalid])])
    assert len(caplog.records) == 1

def test_dtype_all_valid_keys():
    s = summary.summary(minimal_keywords)
    columns = [
        ('REPORTSTEP', 'i4'), ('MINISTEP', 'i4'),
        ('WOPR.W1', 'f4'), ('WOPT.W2', 'f4'),
    ]

    assert s.dtype == np.dtype(columns)
    assert s.pos == [0, 1]

def test_dtype_custom_separator():
    s = summary.summary(minimal_keywords)
    s.dtype_separator = '-'
    columns = [
        ('REPORTSTEP', 'i4'), ('MINISTEP', 'i4'),
        ('WOPR-W1', 'f4'), ('WOPT-W2', 'f4'),
    ]
    assert s.dtype == np.dtype(columns)
    assert s.pos == [0, 1]

def test_dtype_void_wgname():
    keywords = dict(minimal_keywords)
    keywords['DIMENS'  ][0] += 1
    keywords['KEYWORDS'].append('WOPR')
    keywords['WGNAMES' ].append(':+:+:+:+')
    keywords['UNITS'   ].append('SM3/DAY')
    keywords['MEASRMNT'] += ['O:FLOWVO', 'LUME    ']
    keywords['NUMS'    ].append(0)
    s = summary.summary(keywords)
    columns = [
        ('REPORTSTEP', 'i4'), ('MINISTEP', 'i4'),
        ('WOPR.W1', 'f4'), ('WOPT.W2', 'f4'),
    ]
    assert s.dtype == np.dtype(columns)
    assert s.pos == [0, 1]

def test_load_smspec():
    s = summary.load(data / 'simple3.smspec')
    assert s.nlist == 687
    assert s.gridshape == (20, 20, 10)
    assert s.simulator == 'ECLIPSE 100'
