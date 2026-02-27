# This file is part of eyeo Chromium SDK,
# Copyright (C) 2006-present eyeo GmbH
#
# eyeo Chromium SDK is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# eyeo Chromium SDK is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.

from benchmarks import loading_metrics_category

from core import perf_benchmark
from core import platforms

from telemetry import benchmark
from telemetry import story
from telemetry.timeline import chrome_trace_category_filter
from telemetry.timeline import chrome_trace_config
from telemetry.web_perf import timeline_based_measurement
import page_sets


class PerfTestBase(perf_benchmark.PerfBenchmark):
  is_desktop = False

  @classmethod
  def AddBenchmarkCommandLineArgs(cls, parser):
    parser.add_argument('--desktop',
                        action="store_true",
                        help="Mobile/Desktop perf-test switch")

  @classmethod
  def ProcessCommandLineArgs(cls, parser, args):
    if args.desktop:
      cls.is_desktop = True


# "Inspired" by benchmarks/system_health.py.
class PerfTimeTests(PerfTestBase):
  options = {'pageset_repeat': 10}

  def CreateCoreTimelineBasedMeasurementOptions(self):
    cat_filter = chrome_trace_category_filter.ChromeTraceCategoryFilter(
        filter_string='rail,toplevel,uma')
    cat_filter.AddIncludedCategory('eyeo')

    options = timeline_based_measurement.Options(cat_filter)
    loading_metrics_category.AugmentOptionsForLoadingMetrics(options)

    return options

  def SetExtraBrowserOptions(self, options):
    options.flush_os_page_caches_on_start = False
    # Using the software fallback can skew the rendering related metrics. So
    # disable that.
    options.AppendExtraBrowserArgs('--disable-software-compositing-fallback')
    # Force online state for the offline indicator so it doesn't show and affect
    # the benchmarks on bots, which are offline by default.
    options.AppendExtraBrowserArgs(
        '--force-online-connection-state-for-indicator')


class PerfMemoryTests(PerfTestBase):
  options = {'pageset_repeat': 3}

  def CreateCoreTimelineBasedMeasurementOptions(self):
    cat_filter = chrome_trace_category_filter.ChromeTraceCategoryFilter(
        filter_string='-*,disabled-by-default-memory-infra')
    # Needed for the console error metric.
    cat_filter.AddIncludedCategory('v8.console')
    cat_filter.AddIncludedCategory('eyeo')
    options = timeline_based_measurement.Options(cat_filter)
    options.config.enable_android_graphics_memtrack = True
    options.SetTimelineBasedMetrics(['consoleErrorMetric', 'memoryMetric'])
    memory_dump_config = chrome_trace_config.MemoryDumpConfig()
    # Enable for more frequent memory measurements in memory consumption tests:
    # memory_dump_config.AddTrigger('light', 1000)
    options.config.chrome_trace_config.SetMemoryDumpConfig(memory_dump_config)
    return options

  def SetExtraBrowserOptions(self, options):
    # Just before we measure memory we flush the system caches
    # unfortunately this doesn't immediately take effect, instead
    # the next story run is effected. Due to this the first story run
    # has anomalous results. This option causes us to flush caches
    # each time before Chrome starts so we effect even the first story
    # - avoiding the bug.
    options.flush_os_page_caches_on_start = True
    # Force online state for the offline indicator so it doesn't show and affect
    # the benchmarks on bots, which are offline by default.
    options.AppendExtraBrowserArgs(
        '--force-online-connection-state-for-indicator')


class MemoryFilterListFullPagesetAdHeavyDisableAdblock(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_adheavy_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoAdHeavyPageSet(take_memory_measurement=True,
                                        is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetAdHeavyDisableAa(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_adheavy_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoAdHeavyPageSet(take_memory_measurement=True,
                                        is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetAdHeavyDefault(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_adheavy_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoAdHeavyPageSet(take_memory_measurement=True,
                                        is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedDisableAdblock(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSet(take_memory_measurement=True,
                                         is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedDisableAa(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSet(take_memory_measurement=True,
                                         is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedDefault(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSet(take_memory_measurement=True,
                                         is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetSmallDisableAdblock(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_small_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoSmallPageSet(take_memory_measurement=True,
                                      is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetSmallDisableAa(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_small_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoSmallPageSet(take_memory_measurement=True,
                                      is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetSmallDefault(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_small_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoSmallPageSet(take_memory_measurement=True,
                                      is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetSingleDisableAdblock(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_single_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoSinglePageSet(take_memory_measurement=True,
                                       is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetSingleDisableAa(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_single_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoSinglePageSet(take_memory_measurement=True,
                                       is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetSingleDefault(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_single_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoSinglePageSet(take_memory_measurement=True,
                                       is_desktop=self.is_desktop)


class PltFilterListFullPagesetAdHeavyDisableAdblock(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_adheavy_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoAdHeavyPageSet(take_memory_measurement=False,
                                        is_desktop=self.is_desktop)


class PltFilterListFullPagesetAdHeavyDisableAa(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_adheavy_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoAdHeavyPageSet(take_memory_measurement=False,
                                        is_desktop=self.is_desktop)


class PltFilterListFullPagesetAdHeavyDefault(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_adheavy_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoAdHeavyPageSet(take_memory_measurement=False,
                                        is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedDisableAdblock(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSet(take_memory_measurement=False,
                                         is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedDisableAa(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSet(take_memory_measurement=False,
                                         is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedDefault(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSet(take_memory_measurement=False,
                                         is_desktop=self.is_desktop)


class PltFilterListFullPagesetSmallDisableAdblock(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_small_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoSmallPageSet(take_memory_measurement=False,
                                      is_desktop=self.is_desktop)


class PltFilterListFullPagesetSmallDisableAa(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_small_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoSmallPageSet(take_memory_measurement=False,
                                      is_desktop=self.is_desktop)


class PltFilterListFullPagesetSmallDefault(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_small_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoSmallPageSet(take_memory_measurement=False,
                                      is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt1DisableAdblock(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt1_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt1(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt1DisableAa(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt1_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt1(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt1Default(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt1_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt1(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt2DisableAdblock(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt2_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt2(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt2DisableAa(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt2_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt2(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt2Default(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt2_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt2(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt3DisableAdblock(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt3_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt3(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt3DisableAa(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt3_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt3(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt3Default(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt3_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt3(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt4DisableAdblock(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt4_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt4(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt4DisableAa(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt4_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt4(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt4Default(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt4_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt4(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt5DisableAdblock(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt5_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt5(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt5DisableAa(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt5_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt5(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt5Default(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt5_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt5(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt6DisableAdblock(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt6_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt6(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt6DisableAa(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt6_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt6(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class MemoryFilterListFullPagesetExtendedPt6Default(PerfMemoryTests):

  @classmethod
  def Name(cls):
    return "eyeo.memory_full_filter_list_pageset_extended_pt6_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt6(take_memory_measurement=True,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt1DisableAdblock(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt1_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt1(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt1DisableAa(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt1_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt1(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt1Default(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt1_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt1(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt2DisableAdblock(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt2_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt2(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt2DisableAa(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt2_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt2(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt2Default(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt2_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt2(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt3DisableAdblock(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt3_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt3(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt3DisableAa(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt3_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt3(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt3Default(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt3_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt3(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt4DisableAdblock(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt4_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt4(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt4DisableAa(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt4_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt4(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt4Default(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt4_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt4(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt5DisableAdblock(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt5_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt5(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt5DisableAa(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt5_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt5(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt5Default(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt5_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt5(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt6DisableAdblock(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt6_disable_adblock"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt6(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt6DisableAa(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt6_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt6(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)


class PltFilterListFullPagesetExtendedPt6Default(PerfTimeTests):

  @classmethod
  def Name(cls):
    return "eyeo.plt_full_filter_list_pageset_extended_pt6_default"

  def CreateStorySet(self, options):
    return page_sets.EyeoExtendedPageSetPt6(take_memory_measurement=False,
                                            is_desktop=self.is_desktop)
