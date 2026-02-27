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

from page_sets.system_health import loading_stories
from page_sets.system_health import story_tags
from py_utils import discover
from telemetry import story
from telemetry.page import cache_temperature as cache_temperature_module
from telemetry.page import page as page_module


def GetWprArchivesPath(is_desktop=False):
  return ('../eyeo_data/desktop/' if is_desktop else '../eyeo_data/mobile/') + \
    'eyeo_misc_full.json'


class EyeoStorySet(loading_stories._LoadingStory):

  def __init__(self, story_set, take_memory_measurement, is_desktop):
    super(EyeoStorySet, self).__init__(story_set, take_memory_measurement)
    self.is_desktop = is_desktop

  @property
  def cache_temperature(self):
    return cache_temperature_module.WARM_BROWSER


class Load_office_Story2021(EyeoStorySet):
  NAME = 'load:media:_office_:2021'
  URL = 'https://www.office.com/'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.is_desktop:
      if self.load_count % 2 == 0:
        action_runner.WaitForElement(text='Accept')
        action_runner.ClickElement(text='Accept')
      self.load_count += 1


class Load_walmart_Story2021(EyeoStorySet):
  NAME = 'load:media:_walmart_:2021'
  URL = 'https://www.walmart.com/'
  TAGS = [story_tags.YEAR_2021]


class Load_pexels_Story2021(EyeoStorySet):
  NAME = 'load:media:_pexels_:2021'
  URL = 'https://www.pexels.com/discover/'
  TAGS = [story_tags.YEAR_2021]


class Load_britishcouncil_orgStory2021(EyeoStorySet):
  NAME = 'load:media:_britishcouncil_org:2021'
  URL = 'https://www.britishcouncil.org/study-work-abroad/in-uk'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = '#onetrust-accept-btn-handler'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_vegan_Story2021(EyeoStorySet):
  NAME = 'load:media:vegan_:2021'
  URL = 'https://vegan.com/info/eating/'
  TAGS = [story_tags.YEAR_2021]


class Load_pucrs_brStory2021(EyeoStorySet):
  NAME = 'load:media:_pucrs_br:2021'
  URL = 'https://www.pucrs.br/internacional/pucrs-pelo-mundo/'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = '#banner-lgpd-active'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_ups_Story2021(EyeoStorySet):
  NAME = 'load:media:_ups_:2021'
  URL = 'https://www.ups.com/ca/en/Home.page'
  TAGS = [story_tags.YEAR_2021]


class Load1_gogoanime_aiStory2021(EyeoStorySet):
  NAME = 'load:media:1_gogoanime_ai:2021'
  URL = 'https://www1.gogoanime.ai/koi-to-yobu-ni-wa-kimochi-warui-episode-7'
  TAGS = [story_tags.YEAR_2021]


class Load_eclypsia_Story2021(EyeoStorySet):
  NAME = 'load:media:_eclypsia_:2021'
  URL = 'https://www.eclypsia.com/fr/blizzard/actualites/jeff-kaplan-vice-president-de-blizzard-quitte-la-societe-et-laisse-overwatch-a-aaron-keller-32837'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = 'button.css-1litn2c'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_lenta_ruStory2021(EyeoStorySet):
  NAME = 'load:media:lenta_ru:2021'
  URL = 'https://lenta.ru/news/2021/05/10/ne_golodaem/'
  TAGS = [story_tags.YEAR_2021]


class Load_fishki_netStory2021(EyeoStorySet):
  NAME = 'load:media:m_fishki_net:2021'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def __init__(self, story_set, take_memory_measurement, is_desktop):
    self.URL = (
        'https://fishki.net/3741331-memy-s-osuzhdajuwim-kotikom-kotorye-rassmeshat-ljubogo.html'
        if is_desktop else
        'https://m.fishki.net/3741331-memy-s-osuzhdajuwim-kotikom-kotorye-rassmeshat-ljubogo.html'
    )
    super(Load_fishki_netStory2021,
          self).__init__(story_set, take_memory_measurement, is_desktop)

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = '#privacypolicy__close'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_Riau24_comStory2021(EyeoStorySet):
  NAME = 'load:media:Riau24_com:2021'
  TAGS = [story_tags.YEAR_2021]

  def __init__(self, story_set, take_memory_measurement, is_desktop):
    self.URL = 'https://Riau24.com' if is_desktop else 'https://m.Riau24.com'
    super(Load_Riau24_comStory2021,
          self).__init__(story_set, take_memory_measurement, is_desktop)


class Load_lacote_chStory2021(EyeoStorySet):
  NAME = 'load:media:_lacote_ch:2021'
  URL = 'https://www.lacote.ch/articles/economie/'
  TAGS = [story_tags.YEAR_2021]


class Load_novochag_ruStory2021(EyeoStorySet):
  NAME = 'load:media:_novochag_ru:2021'
  URL = 'https://www.novochag.ru/home/pets/dva-goda-nazad-devushka-dala-shans-lysomu-naydenyshu-vot-v-kogo-on-vyros/'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.is_desktop:
      if self.load_count % 2 == 0:
        cookie_button_selector = 'button.cookies-button-confirm'
        action_runner.WaitForElement(selector=cookie_button_selector)
        action_runner.ScrollPageToElement(selector=cookie_button_selector)
        action_runner.ClickElement(selector=cookie_button_selector)
      self.load_count += 1


class Load_extreme_down_tvStory2021(EyeoStorySet):
  NAME = 'load:media:_extreme_down_tv:2021'
  URL = 'https://www.extreme-down.tv/series/vf/82437-ncis-los-angeles-saison-12-french.html'
  TAGS = [story_tags.YEAR_2021]


class Load_freenet_deStory2021(EyeoStorySet):
  NAME = 'load:media:_freenet_de:2021'
  URL = 'https://www.freenet.de/index.html'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = ('button#save' if self.is_desktop else
                                'button.message-button.primary')
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_9gag_Story2021(EyeoStorySet):
  NAME = 'load:media:9gag_:2021'
  URL = 'https://9gag.com/gag/aYoOzyx'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = 'button.css-1k47zha'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_idealista_Story2021(EyeoStorySet):
  NAME = 'load:media:_idealista_:2021'
  URL = 'https://www.idealista.com/venta-viviendas/madrid/centro/lavapies-embajadores/'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = '#didomi-notice-agree-button'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_195sports_Story2021(EyeoStorySet):
  NAME = 'load:media:_195sports_:2021'
  URL = 'https://www.195sports.com/%d8%b3%d9%88%d8%b3%d8%a7-%d9%84%d9%86-%d9%8a%d9%84%d8%b9%d8%a8-%d9%85%d8%b9-%d8%a7%d9%84%d9%85%d9%86%d8%aa%d8%ae%d8%a8-%d8%a7%d9%84%d8%a3%d9%84%d9%85%d8%a7%d9%86%d9%8a-%d8%b1%d8%ba%d9%85-%d8%ad%d8%b5/'
  TAGS = [story_tags.YEAR_2021]


class Load_healthline_Story2021(EyeoStorySet):
  NAME = 'load:media:_healthline_:2021'
  URL = 'https://www.healthline.com/health/video/life-to-the-fullest-with-type-2-diabetes-10#1'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = ('button.css-quk35p'
                                if self.is_desktop else 'button.css-17ksaeu')
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_howstuffworks_Story2021(EyeoStorySet):
  NAME = 'load:media:_howstuffworks_:2021'
  URL = 'https://www.howstuffworks.com/search.php?terms=car'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = ('#onetrust-accept-btn-handler' if
                                self.is_desktop else 'a.banner_continue--3t3Mf')
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_yelp_Story2021(EyeoStorySet):
  NAME = 'load:media:_yelp_:2021'
  URL = 'https://www.yelp.com/biz/my-pie-pizzeria-romana-new-york-2'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = '#onetrust-accept-btn-handler'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_ofeminin_plStory2021(EyeoStorySet):
  NAME = 'load:media:_ofeminin_pl:2021'
  URL = 'https://www.ofeminin.pl/czas-wolny/szukala-ich-cala-polska-do-dzis-ich-nie-odnaleziono-najglosniejsze-zaginiecia-w-kraju/qlnyber'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = 'button.cmp-button_button.cmp-intro_acceptAll '
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_merriam_webster_Story2021(EyeoStorySet):
  NAME = 'load:media:_merriam_webster_:2021'
  URL = 'https://www.merriam-webster.com/dictionary/grade'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = ('#save' if self.is_desktop else
                                '#onetrust-accept-btn-handler')
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_dw_Story2021(EyeoStorySet):
  NAME = 'load:media:_dw_:2021'
  URL = 'https://www.dw.com/en/germany-logs-rise-in-cybercrime-as-pandemic-provides-attack-potential/a-57487775'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = 'a.cookie__btn--ok'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_jneurosci_orgStory2021(EyeoStorySet):
  NAME = 'load:media:_jneurosci_org:2021'
  URL = 'https://www.jneurosci.org/content/early/2021/04/15/JNEUROSCI.2456-20.2021'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = 'button.agree-button'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_euronews_Story2021(EyeoStorySet):
  NAME = 'load:media:_euronews_:2021'
  URL = 'https://www.euronews.com/travel/2021/05/02/wine-tasting-in-the-loire-valley-exploring-the-world-s-favourite-sauvignon-blanc'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = '#didomi-notice-agree-button'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_forums_macrumors_Story2021(EyeoStorySet):
  NAME = 'load:media:forums_macrumors_:2021'
  URL = 'https://forums.macrumors.com/forums/macrumors-com-news-discussion.4/'
  TAGS = [story_tags.YEAR_2021]


class Load_tweakers_netStory2021(EyeoStorySet):
  NAME = 'load:media:tweakers_net:2021'
  URL = 'https://tweakers.net/nieuws/181498/alle-smartphones-oppo-krijgen-drie-jaar-updates-in-plaats-van-twee-jaar.html'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.is_desktop or self.load_count % 2 == 0:
      cookie_button_selector = 'button.ctaButton'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_ellitoral_Story2021(EyeoStorySet):
  NAME = 'load:media:_ellitoral_:2021'
  URL = 'https://www.ellitoral.com/index.php/id_um/297441-reutemann-volvio-a-sufrir-una-hemorragia-en-su-sistema-digestivo-internado-en-rosario-politica-internado-en-rosario.html'
  TAGS = [story_tags.YEAR_2021]


class Load_yandex_ruStory2021(EyeoStorySet):
  NAME = 'load:media:yandex_ru:2021'
  URL = 'https://yandex.ru/video/search?text=iphone&from=tabbar'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = 'button.sc-jrsJCI'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_ria_ruStory2021(EyeoStorySet):
  NAME = 'load:media:ria_ru:2021'
  URL = 'https://ria.ru/'
  TAGS = [story_tags.YEAR_2021]


class Load_genpi_coStory2021(EyeoStorySet):
  NAME = 'load:media:_genpi_co:2021'
  URL = 'https://www.genpi.co/'
  TAGS = [story_tags.YEAR_2021]


class Load_nytimes_Story2021(EyeoStorySet):
  NAME = 'load:media:_nytimes_:2021'
  URL = 'https://www.nytimes.com/'
  TAGS = [story_tags.YEAR_2021]

  def _DidLoadDocument(self, action_runner):
    if self.is_desktop:
      cookie_button_selector = 'button.css-aovwtd'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)


class Load_speedtest_netStory2021(EyeoStorySet):
  NAME = 'load:media:_speedtest_net:2021'
  URL = 'https://www.speedtest.net/'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = '#_evidon-banner-acceptbutton'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_extra_globo_Story2021(EyeoStorySet):
  NAME = 'load:media:extra_globo_:2021'
  URL = 'https://extra.globo.com/economia/emprego/servidor-publico/estado-do-rio-paga-integralmente-salarios-de-julho-dos-servidores-nesta-quarta-feira-dia-14-23872729.html'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = 'button.cookie-banner-lgpd_accept-button'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_OlxStory2021(EyeoStorySet):
  NAME = 'load:media:olx:2021'
  URL = 'https://www.olx.ro/auto-masini-moto-ambarcatiuni/'
  TAGS = [story_tags.YEAR_2021]

  def _DidLoadDocument(self, action_runner):
    cookie_button_selector = '[id="onetrust-accept-btn-handler"]'
    action_runner.WaitForElement(selector=cookie_button_selector)
    action_runner.ScrollPageToElement(selector=cookie_button_selector)
    action_runner.ClickElement(selector=cookie_button_selector)
    action_runner.ScrollPage(use_touch=True, distance=3000)


class Load_techradar_Story2021(EyeoStorySet):
  NAME = 'load:media:_techradar_:2021'
  URL = 'https://www.techradar.com/news/write-once-run-anywhere-google-flutter-20-could-make-every-app-developers-dream-a-reality'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = ('[class=" css-47sehv"]'
                                if self.is_desktop else '[class=" css-flk0bs"]')
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_npr_orgStory2021(EyeoStorySet):
  NAME = 'load:media:_npr_org:2021'
  URL = 'https://www.npr.org/2021/04/19/988628114/europes-top-soccer-teams-announce-new-super-league'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = 'button.user-action--accept'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_youtube_Story2021(EyeoStorySet):
  NAME = 'load:media:_youtube_:2021'
  URL = 'https://www.youtube.com/results?search_query=casey+neistat'
  TAGS = [story_tags.YEAR_2021]


class Loaden_wikipedia_orgStory2021(EyeoStorySet):
  NAME = 'load:media:en_wikipedia_org:2021'
  URL = 'https://en.wikipedia.org/wiki/Adblock_Plus'
  TAGS = [story_tags.YEAR_2021]


class Load_search_yahoo_Story2021(EyeoStorySet):
  NAME = 'load:media:search_yahoo_:2021'
  URL = 'https://search.yahoo.com/search?p=iphone'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.is_desktop or self.load_count % 2 == 0:
      cookie_button_selector = 'button.primary'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_stackoverflow_Story2021(EyeoStorySet):
  NAME = 'load:media:stackoverflow_:2021'
  URL = 'https://stackoverflow.com/questions/66689652/microstack-network-multi-node'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.is_desktop or self.load_count % 2 == 0:
      cookie_button_selector = 'button.js-accept-cookies'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_amazon_Story2021(EyeoStorySet):
  NAME = 'load:media:_amazon_:2021'
  URL = 'https://www.amazon.com/s?k=iphone&ref=nb_sb_noss_2'
  TAGS = [story_tags.YEAR_2021]


class Load_google_Story2021(EyeoStorySet):
  NAME = 'load:media:_google_:2021'
  URL = 'https://www.google.com/search?q=laptops'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if not self.is_desktop and self.load_count % 2 == 0:
      cookie_button_selector = '#zV9nZe'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_thesun_co_ukStory2021(EyeoStorySet):
  NAME = 'load:media:_thesun_co_uk:2021'
  URL = 'https://www.thesun.co.uk/news/14919640/brit-mum-tortured-death-baby-burglars-greece/'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = '#message-button'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_elmundo_esStory2021(EyeoStorySet):
  NAME = 'load:media:_elmundo_es:2021'
  URL = 'https://www.elmundo.es/cronica/2021/05/08/6095e471fdddffb3428b4614.html'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = '#didomi-notice-agree-button'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_jooble_orgStory2021(EyeoStorySet):
  NAME = 'load:media:jooble_org:2021'
  URL = 'https://jooble.org/SearchResult?ukw=d'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.is_desktop or self.load_count % 2 == 0:
      cookie_button_selector = '[class="_3264f button_default button_size_M _2b57b e539c _69517"]'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_meteocity_Story2021(EyeoStorySet):
  NAME = 'load:media:_meteocity_:2021'
  URL = 'https://www.meteocity.com/france/cote-dor-d3023423'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = '#didomi-notice-agree-button'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_msn_Story2021(EyeoStorySet):
  NAME = 'load:media:_msn_:2021'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def __init__(self, story_set, take_memory_measurement, is_desktop):
    self.URL = 'https://www.msn.com/de-de/nachrichten/' + \
      ('politik/inflation-pandemie-krieg-%e2%80%93-in-krisen-zeigt-sich-dass-wir-mehr-staat-brauchen/ar-AAVCvaw?li=BBqg6Q9'
      if is_desktop else
      'coronavirus/falsche-angaben-impf-betr%c3%bcger-tausende-vordr%c3%a4ngler-werden-derzeit-nicht-bestraft/ar-BB1gAVJD?li=BBqg6Q9')
    super(Load_msn_Story2021, self).__init__(story_set, take_memory_measurement,
                                             is_desktop)

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = '#onetrust-accept-btn-handler'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_eldiario_esStory2021(EyeoStorySet):
  NAME = 'load:media:_eldiario_es:2021'
  URL = 'https://www.eldiario.es/politica/'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = (
          '#didomi-notice-agree-button' if self.is_desktop else
          'div.sibbo-panel__aside__buttons > a:nth-child(1)')
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_buzzfeed_Story2021(EyeoStorySet):
  NAME = 'load:media:_buzzfeed_:2021'
  URL = 'https://www.buzzfeed.com/anamariaglavan/best-cheap-bathroom-products?origin=hpp'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = ('button.css-2wu0d3'
                                if self.is_desktop else 'button.css-15dhgct')
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_money_cnn_Story2021(EyeoStorySet):
  NAME = 'load:media:money_cnn_:2021'
  URL = 'https://money.cnn.com/2015/07/30/investing/gold-prices-could-drop-to-350/index.html?iid=hp-toplead-dom'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = 'button#onetrust-accept-btn-handler'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_scotch_ioStory2021(EyeoStorySet):
  NAME = 'load:media:scotch_io:2021'
  URL = 'https://scotch.io/tutorials/build-a-star-rating-component-for-react'
  TAGS = [story_tags.YEAR_2021]


class Load_stars242021(EyeoStorySet):
  NAME = 'load:media:starts24:2021'
  URL = 'https://stars24.cz/aktualne-z-internetu/24254-17-listopad-a-obchody-je-otevreno-od-stredy-bude-platit-kritizovane-opatreni'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.is_desktop or self.load_count % 2 == 0:
      cookie_button_selector = ('button.fc-cta-consent' if self.is_desktop else
                                'button#onetrust-accept-btn-handler')
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_idnes_czStory2021(EyeoStorySet):
  NAME = 'load:media:_idnes_cz:2021'
  URL = 'https://www.idnes.cz/zpravy/zahranicni/jeruzalem-hamas-izrael-rakety-palestina-nepokoje.A210510_175323_zahranicni_jhr'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = ('#didomi-notice-agree-button' if self.is_desktop
                                else 'button#onetrust-accept-btn-handler')
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class Load_5euros_Story2021(EyeoStorySet):
  NAME = 'load:media:5euros_:2021'
  URL = 'https://5euros.com/service/60390/faire-un-montage-video-professionnel-10'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.is_desktop:
      if self.load_count % 2 == 0:
        cookie_button_selector = 'button.cky-btn-accept'
        action_runner.WaitForElement(selector=cookie_button_selector)
        action_runner.ScrollPageToElement(selector=cookie_button_selector)
        action_runner.ClickElement(selector=cookie_button_selector)
      self.load_count += 1


class Load_poder360__brStory2021(EyeoStorySet):
  NAME = 'load:media:_poder360__br:2021'
  URL = 'https://www.poder360.com.br/governo/bolsonaro-nega-orcamento-secreto-e-diz-que-apanha-da-imprensa/'
  TAGS = [story_tags.YEAR_2021]


class Load_petfarmfamily_Story2021(EyeoStorySet):
  NAME = 'load:media:petfarmfamily_:2021'
  URL = 'https://petfarmfamily.com/collections/accessories'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if self.load_count % 2 == 0:
      cookie_button_selector = 'button.react-cookie-law-accept-btn'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    self.load_count += 1


class EyeoExtendedPageSet(story.StorySet):

  def __init__(self, is_desktop, take_memory_measurement=False):
    super(EyeoExtendedPageSet,
          self).__init__(archive_data_file=GetWprArchivesPath(is_desktop),
                         cloud_storage_bucket=None)

    self.AddStory(
        Load_195sports_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_britishcouncil_orgStory2021(self, take_memory_measurement,
                                         is_desktop))
    self.AddStory(Load_dw_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_eldiario_esStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_ellitoral_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_euronews_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_genpi_coStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_novochag_ruStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_healthline_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_howstuffworks_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_idealista_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_idnes_czStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_jneurosci_orgStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_lacote_chStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_meteocity_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(Load_msn_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_nytimes_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_ofeminin_plStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_office_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_speedtest_netStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(Load_stars242021(self, take_memory_measurement, is_desktop))
    self.AddStory(Load_ups_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(Load_yelp_Story2021(self, take_memory_measurement,
                                      is_desktop))
    self.AddStory(
        Load_5euros_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_extra_globo_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_jooble_orgStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_lenta_ruStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_fishki_netStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_money_cnn_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_petfarmfamily_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_search_yahoo_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_tweakers_netStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_Riau24_comStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_vegan_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_yandex_ruStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_npr_orgStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_stackoverflow_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Loaden_wikipedia_orgStory2021(self, take_memory_measurement,
                                      is_desktop))
    self.AddStory(
        Load_amazon_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_google_Story2021(self, take_memory_measurement, is_desktop))
    if not is_desktop:
      self.AddStory(
          Load_youtube_Story2021(self, take_memory_measurement, is_desktop))
      self.AddStory(
          Load_extreme_down_tvStory2021(self, take_memory_measurement,
                                        is_desktop))
      self.AddStory(
          Load_merriam_webster_Story2021(self, take_memory_measurement,
                                         is_desktop))
      self.AddStory(
          Load_poder360__brStory2021(self, take_memory_measurement, is_desktop))
      self.AddStory(
          Load_pucrs_brStory2021(self, take_memory_measurement, is_desktop))
      self.AddStory(
          Load_forums_macrumors_Story2021(self, take_memory_measurement,
                                          is_desktop))
      self.AddStory(
          Load_scotch_ioStory2021(self, take_memory_measurement, is_desktop))
      self.AddStory(
          Load_walmart_Story2021(self, take_memory_measurement, is_desktop))
      self.AddStory(
          Load_ria_ruStory2021(self, take_memory_measurement, is_desktop))
    else:
      self.AddStory(
          Load_buzzfeed_Story2021(self, take_memory_measurement, is_desktop))


class EyeoAdHeavyPageSet(story.StorySet):

  def __init__(self, is_desktop, take_memory_measurement=False):
    super(EyeoAdHeavyPageSet,
          self).__init__(archive_data_file=GetWprArchivesPath(is_desktop),
                         cloud_storage_bucket=None)

    self.AddStory(
        Load_novochag_ruStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_idnes_czStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_lacote_chStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_meteocity_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(Load_msn_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_ofeminin_plStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_jooble_orgStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_lenta_ruStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_fishki_netStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_Riau24_comStory2021(self, take_memory_measurement, is_desktop))
    if not is_desktop:
      self.AddStory(
          Load_merriam_webster_Story2021(self, take_memory_measurement,
                                         is_desktop))
      self.AddStory(
          Load_extreme_down_tvStory2021(self, take_memory_measurement,
                                        is_desktop))


class EyeoExtendedPageSetPt1(story.StorySet):

  def __init__(self, is_desktop, take_memory_measurement=False):
    super(EyeoExtendedPageSetPt1,
          self).__init__(archive_data_file=GetWprArchivesPath(is_desktop),
                         cloud_storage_bucket=None)

    self.AddStory(
        Load_195sports_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_britishcouncil_orgStory2021(self, take_memory_measurement,
                                         is_desktop))
    self.AddStory(Load_dw_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_eldiario_esStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_ellitoral_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_euronews_Story2021(self, take_memory_measurement, is_desktop))
    if not is_desktop:
      self.AddStory(
          Load_extreme_down_tvStory2021(self, take_memory_measurement,
                                        is_desktop))
    else:
      self.AddStory(
          Load_buzzfeed_Story2021(self, take_memory_measurement, is_desktop))


class EyeoExtendedPageSetPt2(story.StorySet):

  def __init__(self, is_desktop, take_memory_measurement=False):
    super(EyeoExtendedPageSetPt2,
          self).__init__(archive_data_file=GetWprArchivesPath(is_desktop),
                         cloud_storage_bucket=None)

    self.AddStory(
        Load_genpi_coStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_novochag_ruStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_healthline_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_howstuffworks_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_idealista_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_idnes_czStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_jneurosci_orgStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_lacote_chStory2021(self, take_memory_measurement, is_desktop))


class EyeoExtendedPageSetPt3(story.StorySet):

  def __init__(self, is_desktop, take_memory_measurement=False):
    super(EyeoExtendedPageSetPt3,
          self).__init__(archive_data_file=GetWprArchivesPath(is_desktop),
                         cloud_storage_bucket=None)

    self.AddStory(
        Load_meteocity_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(Load_msn_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_nytimes_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_ofeminin_plStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_office_Story2021(self, take_memory_measurement, is_desktop))
    if not is_desktop:
      self.AddStory(
          Load_merriam_webster_Story2021(self, take_memory_measurement,
                                         is_desktop))
      self.AddStory(
          Load_poder360__brStory2021(self, take_memory_measurement, is_desktop))
      self.AddStory(
          Load_pucrs_brStory2021(self, take_memory_measurement, is_desktop))


class EyeoExtendedPageSetPt4(story.StorySet):

  def __init__(self, is_desktop, take_memory_measurement=False):
    super(EyeoExtendedPageSetPt4,
          self).__init__(archive_data_file=GetWprArchivesPath(is_desktop),
                         cloud_storage_bucket=None)

    self.AddStory(
        Load_speedtest_netStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(Load_stars242021(self, take_memory_measurement, is_desktop))
    self.AddStory(Load_ups_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(Load_yelp_Story2021(self, take_memory_measurement,
                                      is_desktop))
    self.AddStory(
        Load_5euros_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_extra_globo_Story2021(self, take_memory_measurement, is_desktop))
    if not is_desktop:
      self.AddStory(
          Load_walmart_Story2021(self, take_memory_measurement, is_desktop))
      self.AddStory(
          Load_forums_macrumors_Story2021(self, take_memory_measurement,
                                          is_desktop))


class EyeoExtendedPageSetPt5(story.StorySet):

  def __init__(self, is_desktop, take_memory_measurement=False):
    super(EyeoExtendedPageSetPt5,
          self).__init__(archive_data_file=GetWprArchivesPath(is_desktop),
                         cloud_storage_bucket=None)

    self.AddStory(
        Load_jooble_orgStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_lenta_ruStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_fishki_netStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_money_cnn_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_petfarmfamily_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_search_yahoo_Story2021(self, take_memory_measurement, is_desktop))
    if not is_desktop:
      self.AddStory(
          Load_ria_ruStory2021(self, take_memory_measurement, is_desktop))
      self.AddStory(
          Load_scotch_ioStory2021(self, take_memory_measurement, is_desktop))


class EyeoExtendedPageSetPt6(story.StorySet):

  def __init__(self, is_desktop, take_memory_measurement=False):
    super(EyeoExtendedPageSetPt6,
          self).__init__(archive_data_file=GetWprArchivesPath(is_desktop),
                         cloud_storage_bucket=None)

    self.AddStory(
        Load_tweakers_netStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_Riau24_comStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_vegan_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_yandex_ruStory2021(self, take_memory_measurement, is_desktop))


class EyeoSmallPageSet(story.StorySet):

  def __init__(self, is_desktop, take_memory_measurement=False):
    super(EyeoSmallPageSet,
          self).__init__(archive_data_file=GetWprArchivesPath(is_desktop),
                         cloud_storage_bucket=None)

    self.AddStory(
        Load_npr_orgStory2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_stackoverflow_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Loaden_wikipedia_orgStory2021(self, take_memory_measurement,
                                      is_desktop))
    self.AddStory(
        Load_amazon_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_google_Story2021(self, take_memory_measurement, is_desktop))
    self.AddStory(
        Load_jooble_orgStory2021(self, take_memory_measurement, is_desktop))
    if not is_desktop:
      self.AddStory(
          Load_youtube_Story2021(self, take_memory_measurement, is_desktop))
    else:
      self.AddStory(
          Load_lacote_chStory2021(self, take_memory_measurement, is_desktop))


class EyeoSinglePageSet(story.StorySet):

  def __init__(self, is_desktop, take_memory_measurement=False):
    super(EyeoSinglePageSet,
          self).__init__(archive_data_file=GetWprArchivesPath(is_desktop),
                         cloud_storage_bucket=None)

    self.AddStory(
        Load_fishki_netStory2021(self, take_memory_measurement, is_desktop))
