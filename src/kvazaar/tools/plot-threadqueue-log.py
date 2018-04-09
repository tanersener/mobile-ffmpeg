"""
/*****************************************************************************
 * This file is part of Kvazaar HEVC encoder.
 *
 * Copyright (C) 2013-2015 Tampere University of Technology and others (see
 * COPYING file).
 *
 * Kvazaar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1 as
 * published by the Free Software Foundation.
 *
 * Kvazaar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Kvazaar.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
"""

import numpy as np
import matplotlib.pyplot as plt

import re, weakref

class LogJob:
  def __init__(self, worker_id, enqueue, start, stop, dequeue, description, is_thread_job=True):
    self._worker_id = worker_id
    self._enqueue = enqueue
    self._start = start
    self._stop = stop
    self._dequeue = dequeue
    self._description = description
    self._is_thread_job = is_thread_job

    self._depends = []
    self._rdepends = []

  def add_dependency_on(self, o):
    self._depends.append(weakref.proxy(o))
    o._rdepends.append(weakref.proxy(self))

  def remove_offset(self, offset):
    if self._enqueue is not None:
      self._enqueue -= offset
    self._start -= offset
    self._stop -= offset
    if self._dequeue is not None:
      self._dequeue -= offset

  def get_min_offset(self):
    return min([x for x in [self._enqueue, self._start, self._stop, self._dequeue] if x is not None])

  def _get_properties(self):
    return dict([x.split('=',1) for x in self._description.split(',')])

  def _position_from_str(self, s):
    if re.match('^[0-9]+$', s):
      return int(s)
    else:
      v = [float(x) for x in s.split('-', 1)]
      return (v[0] + v[1]) / 2

  def _height_from_str(self, s):
    if self._is_thread_job:
      diff = 0.2
    else:
      diff = 0.4
    if re.match('^[0-9]+$', s):
      return 1.0 - diff
    else:
      v = [float(x) for x in s.split('-', 1)]
      return (max(v) - min(v) + 1) - diff

  def height(self):
    desc = self._get_properties()
    if 'row' in desc:
      return self._height_from_str(desc['row'])
    elif 'position_y' in desc:
      return self._height_from_str(desc['position_y'])
    else:
      if self._is_thread_job:
        return 0.8
      else:
        return 0.6

  def position_y(self):
    desc = self._get_properties()
    if 'row' in desc:
      return self._position_from_str(desc['row'])
    elif 'position_y' in desc:
      return self._position_from_str(desc['position_y'])
    else:
      return -1

class LogFlush:
  def __init__(self, when):
    self._when = when

  def remove_offset(self, offset):
    self._when -= offset

  def get_min_offset(self):
    return self._when

class LogThread:
  def __init__(self, worker_id, start, stop):
    self._worker_id = worker_id
    self._start = start
    self._stop = stop

  def remove_offset(self, offset):
    self._start -= offset
    self._stop -= offset

  def get_min_offset(self):
    return min([self._start, self._stop])

  def plot(self, ax, i):
    ax.barh(i, self._stop - self._start, left=self._start, height=0.9, align='center',label="test", color='yellow')
    
class IntervalThreadCounter:
  def __init__(self):
    self.interval_starts = []
    self.interval_stops = []
    
  def add_interval(self, start, stop):
    self.interval_starts.append(start)
    self.interval_stops.append(stop)
    self.interval_starts.sort()
    self.interval_stops.sort()
    
  def get_values_xd(self):
    #Double the first and the last items
    xds = sorted([(x,'+') for x in self.interval_starts] + [(x,'-') for x in self.interval_stops])
    return xds
    
  def get_values_x(self):
    xs = []
    for x in self.get_values_xd():
      xs.append(x[0])
      xs.append(x[0])
    return xs
  
  def get_values_y(self):
    xds = self.get_values_xd()
    ys = []
    counter = 0
    for xd in xds:
      ys.append(counter)
      
      if xd[1] == '+':
        counter += 1
      elif xd[1] == '-':
        counter -= 1
      else:
        assert False
      
      ys.append(counter)
      
    return ys
    
  def clamp(self, v, minval, maxval):
    if v < minval:
      return minval
    if v > maxval:
      return maxval
    return v
    
  def get_values_uniform_xy(self, kernel_size, steps):
    kernel_size=float(kernel_size)
    xchs = self.get_values_x()
    ychs = self.get_values_y()
    
    minval = xchs[0] - kernel_size
    maxval = xchs[-1] + kernel_size
    
    pos = minval
    
    xvalues = []
    yvalues = []

    while pos < maxval:
      value = 0
      for i in range(1,len(xchs)-1):
        if xchs[i] < pos - kernel_size:
          continue
          
        v1 = self.clamp(xchs[i-1], pos - kernel_size/2., pos+kernel_size/2.)
        v2 = self.clamp(xchs[i], pos - kernel_size/2., pos+kernel_size/2.)
        
        diff=v2-v1
        
        value += diff*ychs[i]/kernel_size
        
        if xchs[i] > pos + kernel_size:
          break
          
      xvalues.append(pos)
      yvalues.append(value)
      
      pos += kernel_size/steps
      
    return xvalues, yvalues
    
      
    
    

class LogParser:
  def _parse_time(self, base, sign, value):
    if sign == '+':
      return base + float(value)
    else:
      return float(value)

  def __init__(self, filename):
    re_thread = re.compile(r'^\t([0-9]+)\t-\t([0-9\.]+)\t(\+?)([0-9\.]+)\t-\tthread$')
    re_job = re.compile(r'^([^\t]+)\t([0-9]+)\t([0-9\.]+)\t(\+?)([0-9\.]+)\t(\+?)([0-9\.]+)\t(\+?)([0-9\.]+)\t(.*)$')
    re_dep = re.compile(r'^(.+)->(.+)$')
    re_flush = re.compile(r'^\t\t-\t-\t([0-9\.]+)\t-\tFLUSH$')
    re_other_perf = re.compile(r'^\t([0-9]*)\t-\t([0-9\.]+)\t(\+?)([0-9\.]*)\t-\t(.*)$')

    objects_cache = {}
    deps_cache = []
    objects = []
    threads = {}

    for line in open(filename,'r').readlines():
      m = re_thread.match(line)
      if m:
        g = m.groups()

        thread_id = int(g[0])
        start = self._parse_time(0, '', g[1])
        stop = self._parse_time(start, g[2], g[3])

        threads[thread_id] = LogThread(thread_id, start, stop)
        continue

      m = re_flush.match(line)
      if m:
        g = m.groups()
        when = self._parse_time(0, '', g[0])
        objects.append(LogFlush(when))

        for g in deps_cache:
          objects_cache[g[1]].add_dependency_on(objects_cache[g[0]])

        #clear object cache
        objects_cache = {}
        deps_cache = []
        continue

      m = re_job.match(line)
      if m:
        g = m.groups()
        worker_id = int(g[1])
        enqueue = self._parse_time(0, '', g[2])
        start = self._parse_time(enqueue, g[3], g[4])
        stop = self._parse_time(start, g[5], g[6])
        dequeue = self._parse_time(stop, g[7], g[8])
        description = g[9]

        value = LogJob(worker_id, enqueue, start, stop, dequeue, description)
        objects.append(value)
        objects_cache[g[0]] = value
        continue

      m = re_dep.match(line)
      if m:
        g = m.groups()
        deps_cache.append((g[0],g[1]))
        continue

      m = re_other_perf.match(line)
      if m:
        g = m.groups()
        if g[0] == '':
          worker_id = None
        else:
          worker_id = int(g[0])

        start = self._parse_time(0, '', g[1])
        stop = self._parse_time(start, g[2], g[3])

        objects.append(LogJob(worker_id, None, start, stop, None, g[4], False))
        continue

      raise ValueError("Unknown line:", line)

    assert len(threads) + len(objects) > 0
    self._threads = threads
    self._objects = objects

    #Remove offsets
    offset = min([x.get_min_offset() for x in self._threads.values()] + [x.get_min_offset() for x in self._objects])

    for x in self._threads.values() + self._objects:
      x.remove_offset(offset)

  def plot_threads(self):
    fig = plt.figure()
    ax=fig.gca()
    yticks = {}
    for k in sorted(self._threads.keys()):
      v = self._threads[k]
      v.plot(ax, -k)
      yticks[-k] = 'Thread {0}'.format(k)

    for o in self._objects:
      if isinstance(o, LogJob):
        ax.barh(-o._worker_id, o._stop - o._start, left=o._start, height=0.8, align='center',label="test", color='green')

      if isinstance(o, LogFlush):
        ax.axvline(o._when)

    for o in self._objects:
      if isinstance(o, LogJob):
        for o2 in o._depends:
          ax.plot([o2._stop, o._start], [-o2._worker_id, -o._worker_id], linewidth=2, color='r')

    plt.yticks( yticks.keys(), yticks.values() )
    fig.show()
    plt.show()

  def get_color(self, i, is_thread_job=True):
    if i is None:
      return 'w'
    if is_thread_job:
      color_keys = ['#ff0000', '#00ff00', '#0000ff', '#ffff00', '#ff00ff', '#00ffff']
    else:
      color_keys = ['#ffaaaa', '#aaffaa', '#aaaaff', '#ffffaa', '#ffaaff', '#aaffff']

    return color_keys[i%len(color_keys)]

  def plot_picture_wise_wpp(self):
    fig = plt.figure()
    ax=fig.gca()

    yticks = {}
    
    #first draw usage
    itc = IntervalThreadCounter()
    for o in self._objects:
      if isinstance(o, LogJob) and o._is_thread_job:
        itc.add_interval(o._start, o._stop)
    
    #exact plot
    ax.plot(itc.get_values_x(), [y+1.5 for y in itc.get_values_y()])
    vx,vy = itc.get_values_uniform_xy(0.01,10)
    ax.plot(vx, [y+1.5 for y in vy], 'r')
    
    for y in set(itc.get_values_y()):
      yticks[y+1.5] = '{0}'.format(y)
    
    

    #first draw threads
    for o in self._objects:
      if isinstance(o, LogJob) and o._is_thread_job:
        y = o.position_y()
        height = o.height()
        ax.barh(-y, o._stop - o._start, left=o._start, height=height, align='center', color=self.get_color(o._worker_id))
        if y % 1 <= 0.0001:
          yticks[int(-y)]= int(y)

    #then jobs
    for o in self._objects:
      if isinstance(o, LogJob) and not o._is_thread_job:
        y = o.position_y()
        height = o.height()
        ax.barh(-y, o._stop - o._start, left=o._start, height=height, align='center', color=self.get_color(o._worker_id, False))
        if y % 1 <= 0.0001:
          yticks[int(-y)]= int(y)

    for o in self._objects:
      if isinstance(o, LogJob):
        for o2 in o._depends:
          ax.plot([o2._stop, o._start], [-int(o2.position_y()), -int(o.position_y())] , linewidth=1, color='k')

    for y in yticks.keys():
      if y<1.5:
        ax.axhline(y+0.5, color='k')
        if y - 1 not in yticks.keys():
          ax.axhline(y-0.5, color='k')
      else:
        ax.axhline(y, color='k', linestyle='dotted')
      if y == 1:
        yticks[y] = "None"

    plt.yticks( yticks.keys(), yticks.values() )

    for o in self._objects:
      if isinstance(o, LogFlush):
        ax.axvline(o._when)


    ax.set_xlabel("Time [s]")
    ax.set_ylabel("LCU y coordinate")

    fig.show()
    plt.show()


  def plot_animation(self):
    pass


if __name__ == '__main__':
  import sys
  if len(sys.argv) > 1:
    l = LogParser(sys.argv[1])
  else:
    l = LogParser('threadqueue.log')
  l.plot_picture_wise_wpp()
  #l.plot_threads()
