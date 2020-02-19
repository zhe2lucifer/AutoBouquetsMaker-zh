# -*- coding: utf-8 -*-
# for localized messages
from . import _

from Screens.Screen import Screen
from Screens.MessageBox import MessageBox
from Components.ActionMap import ActionMap
from Components.Button import Button
from Components.config import config, configfile, ConfigYesNo, ConfigSelection, getConfigListEntry
from Components.ConfigList import ConfigListScreen
from Components.Sources.StaticText import StaticText
from Components.Label import Label
from Components.NimManager import nimmanager
from enigma import eTimer

from skin_templates import skin_setup
from scanner.manager import Manager
from scanner.providerconfig import ProviderConfig
import log
import itertools

class AutoBouquetsMaker_ProvidersSetup(ConfigListScreen, Screen):
# Note to skinners: no need to skin this screen if you have skinned the screen 'AutoBouquetsMaker_Setup'.
	skin = skin_setup()

	ABM_BOUQUET_PREFIX = "userbouquet.abm."

	try: # Work-around to get OpenSPA working
		from boxbranding import getImageDistro
		if getImageDistro() == 'openspa':
			def keyLeft(self):
				ConfigListScreen.keyLeft(self)
				self.changedEntry()

			def keyRight(self):
				ConfigListScreen.keyRight(self)
				self.changedEntry()
	except:
		pass

	def __init__(self, session):
		Screen.__init__(self, session)
		self.session = session
		self.skinName = "AutoBouquetsMaker_Setup"
		self.setup_title = _("AutoBouquetsMaker Providers")
		Screen.setTitle(self, self.setup_title)

		self.onChangedEntry = [ ]
		self.list = []
		ConfigListScreen.__init__(self, self.list, session = self.session, on_change = self.changedEntry)

		self.activityTimer = eTimer()
		self.activityTimer.timeout.get().append(self.prepare)

		self["actions"] = ActionMap(["SetupActions", 'ColorActions', 'VirtualKeyboardActions', "MenuActions"],
		{
			"ok": self.keySave,
			"cancel": self.keyCancel,
			"red": self.keyCancel,
			"green": self.keySave,
			"menu": self.keyCancel,
		}, -2)

		self["key_red"] = Button(_("Cancel"))
		self["key_green"] = Button(_("Save"))
		self["pleasewait"] = Label()
		self["description"] = Label("")

		self.onLayoutFinish.append(self.populate)

	def populate(self):
		self["actions"].setEnabled(False)
		self["pleasewait"].setText(_("Please wait..."))
		self.activityTimer.start(1)

	def prepare(self):
		self.activityTimer.stop()
		self.providers = Manager().getProviders()
		self.providers_configs = {}
		self.providers_area = {}
		self.providers_swapchannels = {}
		self.providers_makemain = {}
		self.providers_custommain = {}
		self.providers_makesections = {}
		self.providers_makehd = {}
		self.providers_makefta = {}
		self.providers_makeftahd = {}
		self.providers_FTA_only = {}
		self.providers_order = []
		self.orbital_supported = []

		# get supported orbital positions
		dvbs_nims = nimmanager.getNimListOfType("DVB-S")
		for nim in dvbs_nims:
			sats = nimmanager.getSatListForNim(nim)
			for sat in sats:
				if sat[0] not in self.orbital_supported:
					self.orbital_supported.append(sat[0])

		self.dvbc_nims = []
		self.dvbt_nims = []
		try: # OpenPLi Hot Switch compatible image
			for nim in nimmanager.nim_slots:
				if nim.config_mode != "nothing":
					if "DVB-C" in [x[:5] for x in nim.getTunerTypesEnabled()]:
						self.dvbc_nims.append(nim.slot)
					if "DVB-T" in [x[:5] for x in nim.getTunerTypesEnabled()]:
						self.dvbt_nims.append(nim.slot)
		except AttributeError:
			try:
				for nim in nimmanager.nim_slots:
					if nim.config_mode != "nothing":
						if nim.isCompatible("DVB-C") or (nim.isCompatible("DVB-S") and nim.canBeCompatible("DVB-C")):
							self.dvbc_nims.append(nim.slot)
						if nim.isCompatible("DVB-T") or (nim.isCompatible("DVB-S") and nim.canBeCompatible("DVB-T")):
							self.dvbt_nims.append(nim.slot)
			except AttributeError: # OpenATV > 5.3
				for nim in nimmanager.nim_slots:
					if nim.canBeCompatible("DVB-C") and nim.config_mode_dvbc != "nothing":
						self.dvbc_nims.append(nim.slot)
					if nim.canBeCompatible("DVB-T") and nim.config_mode_dvbt != "nothing":
						self.dvbt_nims.append(nim.slot)

		# dependent providers
		self.dependents_list = []
		for provider_key in self.providers:
			if len(self.providers[provider_key]["dependent"]) > 0 and self.providers[provider_key]["dependent"] in self.providers:
				self.dependents_list.append(provider_key)


		# read providers configurations
		providers_tmp_configs = {}
		providers_tmp = config.autobouquetsmaker.providers.value.split("|")
		for provider_tmp in providers_tmp:
			provider_config = ProviderConfig(provider_tmp)

			if not provider_config.isValid():
				continue

			if provider_config.getProvider() not in self.providers:
				continue
			if self.providers[provider_config.getProvider()]["streamtype"] == 'dvbs' and self.providers[provider_config.getProvider()]["transponder"]["orbital_position"] not in self.orbital_supported:
				continue
			if self.providers[provider_config.getProvider()]["streamtype"] == 'dvbc' and len(self.dvbc_nims) <= 0:
				continue
			if self.providers[provider_config.getProvider()]["streamtype"] == 'dvbt' and len(self.dvbt_nims) <= 0:
				continue

			self.providers_order.append(provider_config.getProvider())
			providers_tmp_configs[provider_config.getProvider()] = provider_config

		# get current bouquets list (for custom main)
		bouquets = Manager().getBouquetsList()
		bouquets_list = []

		if bouquets["tv"] is not None:
			for bouquet in bouquets["tv"]:
				if bouquet["filename"][:len(self.ABM_BOUQUET_PREFIX)] == self.ABM_BOUQUET_PREFIX:
					continue
				bouquets_list.append((bouquet["filename"], bouquet["name"]))

		# build providers configurations
		for provider in self.providers.keys():
			self.providers_configs[provider] = ConfigYesNo(default = (provider not in self.dependents_list and provider in providers_tmp_configs.keys()))
			self.providers_swapchannels[provider] = ConfigYesNo(default = (provider in providers_tmp_configs and providers_tmp_configs[provider].isSwapChannels()))

			custom_bouquets_exists = False
			self.providers_makemain[provider] = None
			self.providers_custommain[provider] = None
			self.providers_makesections[provider] = None
			self.providers_makehd[provider] = None
			self.providers_makefta[provider] = None
			self.providers_makeftahd[provider] = None

			if len(self.providers[provider]["sections"].keys()) > 1:	# only if there's more than one section
				sections_default = True
				if provider in providers_tmp_configs:
					sections_default = providers_tmp_configs[provider].isMakeSections()
				self.providers_makesections[provider] = ConfigYesNo(default = sections_default)
				custom_bouquets_exists = True

			if self.providers[provider]["protocol"] != "fastscan":	# fastscan doesn't have enough information to make HD and/or FTA bouquets
				hd_default = True
				fta_default = True
				ftahd_default = True
				if provider in providers_tmp_configs:
					hd_default = providers_tmp_configs[provider].isMakeHD()
					fta_default = providers_tmp_configs[provider].isMakeFTA()
					ftahd_default = providers_tmp_configs[provider].isMakeFTAHD()
				self.providers_makehd[provider] = ConfigYesNo(default = hd_default)
				self.providers_makefta[provider] = ConfigYesNo(default = fta_default)
				self.providers_makeftahd[provider] = ConfigYesNo(default = ftahd_default)
				custom_bouquets_exists = True

			if sorted(self.providers[provider]["sections"].keys())[0] > 1:
				makemain_default = "no"
				makemain_list = [("yes", _("yes (all channels)"))]
				if self.providers[provider]["protocol"] != "fastscan":
					makemain_list.append(("hd", _("yes (only HD)")))
					makemain_list.append(("ftahd", _("yes (only FTA HD)")))

				if provider not in providers_tmp_configs and self.providers[provider]["protocol"] == "sky":
					makemain_default = "ftahd"	# FTA HD only as default
				elif provider not in providers_tmp_configs:
					makemain_default = "yes"	# enabled as default

				if provider in providers_tmp_configs and providers_tmp_configs[provider].isMakeNormalMain():
					makemain_default = "yes"

				if self.providers[provider]["protocol"] != "fastscan":
					if provider in providers_tmp_configs and providers_tmp_configs[provider].isMakeHDMain():
						makemain_default = "hd"
					if provider in providers_tmp_configs and providers_tmp_configs[provider].isMakeFTAHDMain():
						makemain_default = "ftahd"

				if len(bouquets_list) > 0 and config.autobouquetsmaker.placement.getValue() == 'top':
					makemain_list.append(("custom", _("yes (custom)")))
					if provider in providers_tmp_configs and providers_tmp_configs[provider].isMakeCustomMain():
						makemain_default = "custom"

					bouquet_default = bouquets_list[0][0]
					if provider in providers_tmp_configs:
						for bouquet_entry in bouquets_list:
							if bouquet_entry[0] == providers_tmp_configs[provider].getCustomFilename():
								bouquet_default = bouquet_entry[0]
								break

					self.providers_custommain[provider] = ConfigSelection(default = bouquet_default, choices = bouquets_list)

				makemain_list.append(("no", _("no")))
				self.providers_makemain[provider] = ConfigSelection(default = makemain_default, choices = makemain_list)

			elif custom_bouquets_exists:
				makemain_default = "no"
				if provider not in providers_tmp_configs:
					makemain_default = "yes"
				if provider in providers_tmp_configs and providers_tmp_configs[provider].isMakeNormalMain():
					makemain_default = "yes"
				self.providers_makemain[provider] = ConfigSelection(default = makemain_default, choices = [("yes", _("yes")), ("no", _("no"))])

			arealist = []
			bouquets = self.providers[provider]["bouquets"]
			for bouquet in bouquets.keys():
				arealist.append((bouquet, self.providers[provider]["bouquets"][bouquet]["name"]))
			arealist.sort()
			if len(self.providers[provider]["bouquets"]) > 0: # provider has area list
				default_area = None
				if provider in providers_tmp_configs:
					default_area = providers_tmp_configs[provider].getArea()
				self.providers_area[provider] = ConfigSelection(default = default_area, choices = arealist)

			# FTA only
			FTA_only = config.autobouquetsmaker.FTA_only.value.split("|")
			FTA = self.providers[provider]["protocol"] != "fastscan" and config.autobouquetsmaker.level.value == "expert" and provider in FTA_only
			self.providers_FTA_only[provider] = ConfigYesNo(default = FTA)

		self.createSetup()
		self["pleasewait"].hide()
		self["actions"].setEnabled(True)

	def providerKeysInNameOrder(self, providers):
		temp = []
		for provider in providers.keys():
			temp.append((provider, providers[provider]["name"]))
		return [i[0] for i in sorted(temp, key=lambda p: p[1].lower().decode('ascii','ignore'))]

	def createSetup(self):
		self.editListEntry = None
		self.list = []
		providers_enabled = []
		providers_already_loaded = []
		indent = '-  '
		for provider in self.providerKeysInNameOrder(self.providers):
			if provider in self.dependents_list:
				continue
			if self.providers[provider]["streamtype"] == 'dvbs' and self.providers[provider]["transponder"]["orbital_position"] not in self.orbital_supported:
				continue
			if self.providers[provider]["streamtype"] == 'dvbc' and len(self.dvbc_nims) <= 0:
				continue
			if self.providers[provider]["streamtype"] == 'dvbt' and len(self.dvbt_nims) <= 0:
				continue
			if self.providers[provider]["name"] in providers_already_loaded:
				continue
			else:
				providers_already_loaded.append(self.providers[provider]["name"])

			self.list.append(getConfigListEntry(self.providers[provider]["name"], self.providers_configs[provider], _("This option enables the current selected provider.")))
			if self.providers_configs[provider].value:
				if len(self.providers[provider]["bouquets"]) > 0:
					self.list.append(getConfigListEntry(indent + _("Region"), self.providers_area[provider], _("This option allows you to choose what region of the country you live in, so it populates the correct channels for your region.")))

				if config.autobouquetsmaker.level.value == "expert":
					# fta only
					if self.providers[provider]["protocol"] != "fastscan":
						self.list.append(getConfigListEntry(indent + _("FTA only"), self.providers_FTA_only[provider], _("This affects all bouquets. Select 'no' to scan in all services. Select 'yes' to skip encrypted ones.")))

					if self.providers_makemain[provider]:
						self.list.append(getConfigListEntry(indent + _("Create main bouquet"), self.providers_makemain[provider], _('This option has several choices "Yes", (create a bouquet with all the channels in it), "Yes HD only", (will group all HD channels into this bouquet), "Custom", (allows you to select your own bouquet), "No", (do not use a main bouquet)')))

					if self.providers_custommain[provider] and self.providers_makemain[provider] and self.providers_makemain[provider].value == "custom":
						self.list.append(getConfigListEntry(indent + _("Custom bouquet for main"), self.providers_custommain[provider], _("Select your own bouquet from the list, please note that the only the first 100 channels for this bouquet will be used.")))

					if self.providers_makesections[provider]:
						self.list.append(getConfigListEntry(indent + _("Create sections bouquets"), self.providers_makesections[provider], _("This option will create bouquets for each type of channel, ie Entertainment, Movies, Documentary.")))

					if self.providers_makehd[provider] and (self.providers_makemain[provider] is None or self.providers_makemain[provider].value != "hd"):
						self.list.append(getConfigListEntry(indent + _("Create HD bouquet"), self.providers_makehd[provider], _("This option will create a High Definition bouquet, it will group all HD channels into this bouquet.")))

					if self.providers_makefta[provider] and not self.providers_FTA_only[provider].value:
						self.list.append(getConfigListEntry(indent + _("Create FTA bouquet"), self.providers_makefta[provider], _("This option will create a FreeToAir bouquet, it will group all free channels into this bouquet.")))

					if self.providers_makeftahd[provider] and (self.providers_makemain[provider] is None or self.providers_makemain[provider].value != "ftahd") and not self.providers_FTA_only[provider].value:
						self.list.append(getConfigListEntry(indent + _("Create FTA HD bouquet"), self.providers_makeftahd[provider], _("This option will create a FreeToAir High Definition bouquet, it will group all FTA HD channels into this bouquet.")))

					if ((self.providers_makemain[provider] and self.providers_makemain[provider].value == "yes") or (self.providers_makesections[provider] and self.providers_makesections[provider].value == True)) and len(self.providers[provider]["swapchannels"]) > 0:
						self.list.append(getConfigListEntry(indent + _("Swap channels"), self.providers_swapchannels[provider], _("This option will swap SD versions of channels with HD versions. (eg BBC One SD with BBC One HD, Channel Four SD with with Channel Four HD)")))

				providers_enabled.append(provider)

		for provider in providers_enabled:
			if provider not in self.providers_order:
				self.providers_order.append(provider)

		for provider in self.providers_order:
			if provider not in providers_enabled:
				self.providers_order.remove(provider)

		self["config"].list = self.list
		self["config"].setList(self.list)

	# for summary:
	def changedEntry(self):
		self.item = self["config"].getCurrent()
		for x in self.onChangedEntry:
			x()
		try:
			if isinstance(self["config"].getCurrent()[1], ConfigYesNo) or isinstance(self["config"].getCurrent()[1], ConfigSelection):
				self.createSetup()
		except:
			pass

	def getCurrentEntry(self):
		return self["config"].getCurrent() and str(self["config"].getCurrent()[0]) or ""

	def getCurrentValue(self):
		return self["config"].getCurrent() and str(self["config"].getCurrent()[1].getText()) or ""

	def getCurrentDescription(self):
		return self["config"].getCurrent() and len(self["config"].getCurrent()) > 2 and self["config"].getCurrent()[2] or ""

	def createSummary(self):
		return SetupSummary

	def saveAll(self):
		for x in self["config"].list:
			x[1].save()

		FTA_only = []

		config_string = ""
		for provider in self.providers_order:
			if self.providers_configs[provider].value:
				if len(config_string) > 0:
					config_string += "|"

				provider_config = ProviderConfig()
				provider_config.unsetAllFlags()

				provider_config.setProvider(provider)
				if len(self.providers[provider]["bouquets"]) > 0:
					provider_config.setArea(self.providers_area[provider].value)

				if self.providers_makemain[provider] is None or self.providers_makemain[provider].value == "yes":
					provider_config.setMakeNormalMain()
				elif self.providers_makemain[provider].value == "hd":
					provider_config.setMakeHDMain()
				elif self.providers_makemain[provider].value == "ftahd":
					provider_config.setMakeFTAHDMain()
				elif self.providers_makemain[provider].value == "custom":
					provider_config.setMakeCustomMain()
					provider_config.setCustomFilename(self.providers_custommain[provider].value)

				if self.providers_makesections[provider] and self.providers_makesections[provider].value:
					provider_config.setMakeSections()

				if self.providers_makehd[provider] and self.providers_makehd[provider].value and (self.providers_makemain[provider] is None or self.providers_makemain[provider].value != "hd"):
					provider_config.setMakeHD()

				if self.providers_makefta[provider] and self.providers_makefta[provider].value and not self.providers_FTA_only[provider].value:
					provider_config.setMakeFTA()

				if self.providers_makeftahd[provider] and self.providers_makeftahd[provider].value and (self.providers_makemain[provider] is None or self.providers_makemain[provider].value != "ftahd") and not self.providers_FTA_only[provider].value:
					provider_config.setMakeFTAHD()

				if self.providers_swapchannels[provider] and self.providers_swapchannels[provider].value:
					provider_config.setSwapChannels()

				config_string += provider_config.serialize()

				if self.providers_FTA_only[provider].value:
					FTA_only.append(provider)

		# fta only
		config.autobouquetsmaker.FTA_only.value = ''
		if FTA_only:
			config.autobouquetsmaker.FTA_only.value = '|'.join(FTA_only)
		config.autobouquetsmaker.FTA_only.save()

		config.autobouquetsmaker.providers.value = config_string
		config.autobouquetsmaker.providers.save()
		configfile.save()

	# keySave and keyCancel are just provided in case you need them.
	# you have to call them by yourself.
	def keySave(self):
		self.saveAll()
		self.close()

	def cancelConfirm(self, result):
		if not result:
			return
		for x in self["config"].list:
			x[1].cancel()
		self.close()

	def keyCancel(self):
		if self["config"].isChanged():
			self.session.openWithCallback(self.cancelConfirm, MessageBox, _("Really close without saving settings?"))
		else:
			self.close()

class AutoBouquetsMaker_Setup(ConfigListScreen, Screen):
	skin = skin_setup()

	def __init__(self, session):
		Screen.__init__(self, session)
		self.session = session
		self.setup_title = _("AutoBouquetsMaker Configure")
		Screen.setTitle(self, self.setup_title)

		self.onChangedEntry = [ ]
		self.list = []
		ConfigListScreen.__init__(self, self.list, session = self.session, on_change = self.changedEntry)

		self.activityTimer = eTimer()
		self.activityTimer.timeout.get().append(self.prepare)

		self["actions"] = ActionMap(["SetupActions", 'ColorActions', 'VirtualKeyboardActions', "MenuActions"],
		{
			"ok": self.keyOk,
			"cancel": self.keyCancel,
			"red": self.keyCancel,
			"green": self.keySave,
			"menu": self.keyCancel,
		}, -2)

		self["key_red"] = Button(_("Cancel"))
		self["key_green"] = Button(_("Save"))
		self["description"] = Label("")
		self["pleasewait"] = Label()

		self.onLayoutFinish.append(self.populate)

	def populate(self):
		self["actions"].setEnabled(False)
		self["pleasewait"].setText(_("Please wait..."))
		self.activityTimer.start(1)

	def prepare(self):
		self.activityTimer.stop()
		bouquets = Manager().getBouquetsList()
		bouquets_list = []
		bouquet_default = None

		self.createSetup()
		self["pleasewait"].hide()
		self["actions"].setEnabled(True)

	def createSetup(self):
		self.editListEntry = None
		self.list = []
		indent = '-  '

#		self.list.append(getConfigListEntry(_("Setup mode"), config.autobouquetsmaker.level, _("Choose which level of settings to display. 'Expert'-level shows all items, this also adds more options in the providers menu.")))
		self.list.append(getConfigListEntry(_("Schedule scan"), config.autobouquetsmaker.schedule, _("Allows you to set a schedule to perform a scan ")))
		if config.autobouquetsmaker.schedule.getValue():
			self.list.append(getConfigListEntry(indent + _("Schedule time of day"), config.autobouquetsmaker.scheduletime, _("Set the time of day to perform a scan.")))
			self.list.append(getConfigListEntry(indent + _("Schedule days of the week"), config.autobouquetsmaker.dayscreen, _("Press OK to select which days to perform a scan.")))
			self.list.append(getConfigListEntry(indent + _("Schedule wake from deep standby"), config.autobouquetsmaker.schedulewakefromdeep, _("If the receiver is in 'Deep Standby' when the schedule is due, wake it up to perform a scan.")))
			if config.autobouquetsmaker.schedulewakefromdeep.getValue():
				self.list.append(getConfigListEntry(indent + _("Schedule return to deep standby"), config.autobouquetsmaker.scheduleshutdown, _("If the receiver was woken from 'Deep Standby' and is currently in 'Standby' and no recordings are in progress return it to 'Deep Standby' once the scan has completed.")))
		if config.autobouquetsmaker.level.value == "expert":
			self.list.append(getConfigListEntry(_("Keep all non-ABM bouquets"), config.autobouquetsmaker.keepallbouquets, _("When disabled this will enable the 'Keep bouquets' option in the main menu, allowing you to hide some 'existing' bouquets.")))
			self.list.append(getConfigListEntry(_("Add provider name to bouquets"), config.autobouquetsmaker.addprefix, _("This option will add the provider's name to bouquet names.")))
			self.list.append(getConfigListEntry(_("Add provider markers"), config.autobouquetsmaker.markersinindex, _("This option places markers in the bouquet index to group all bouquets of each provider.")))
			if config.autobouquetsmaker.markersinindex.getValue():
				self.list.append(getConfigListEntry(indent + _("Style of provider marker"), config.autobouquetsmaker.indexmarkerstyle, _("Choose the style of markers that separate one provider from another in bouquet indexes.")))
			self.list.append(getConfigListEntry(_("Style of bouquet marker"), config.autobouquetsmaker.bouquetmarkerstyle, _("Choose the style of the markers that separate channels into groups in the channel lists.")))
			self.list.append(getConfigListEntry(_("Place bouquets at"), config.autobouquetsmaker.placement, _("This option will allow you choose where to place the created bouquets.")))
			self.list.append(getConfigListEntry(_("Skip services on not configured sats"), config.autobouquetsmaker.skipservices, _("If a service is carried on a satellite that is not configured, 'yes' means the channel will not appear in the channel list, 'no' means the channel will show in the channel list but be greyed out and not be accessible.")))
			self.list.append(getConfigListEntry(_("Include 'not indexed' channels"), config.autobouquetsmaker.showextraservices, _("When a search finds extra channels that do not have an allocated channel number, 'yes' will add these at the end of the channel list, and 'no' means these will not be included.")))
			self.list.append(getConfigListEntry(_("Extra debug"), config.autobouquetsmaker.extra_debug, _("This feature is for development only. Requires debug logs to be enabled or enigma2 to be started in console mode.")))
			self.list.append(getConfigListEntry(_("Show DVB-T frequency finder"), config.autobouquetsmaker.frequencyfinder, _('Select "yes" to show the "DVB-T frequency finder" tool in the main menu. This tool is used to create a working provider file for difficult areas of the UK, e.g. areas covered by repeaters, etc.')))
		self.list.append(getConfigListEntry(_("Show in extensions"), config.autobouquetsmaker.extensions, _("When enabled, allows you start a scan from the extensions list.")))

		self["config"].list = self.list
		self["config"].setList(self.list)

	# for summary:
	def changedEntry(self):
		self.item = self["config"].getCurrent()
		for x in self.onChangedEntry:
			x()
		try:
			if isinstance(self["config"].getCurrent()[1], ConfigYesNo) or isinstance(self["config"].getCurrent()[1], ConfigSelection):
				self.createSetup()
		except:
			pass

	def getCurrentEntry(self):
		return self["config"].getCurrent() and str(self["config"].getCurrent()[0]) or ""

	def getCurrentValue(self):
		return self["config"].getCurrent() and str(self["config"].getCurrent()[1].getText()) or ""

	def getCurrentDescription(self):
		return self["config"].getCurrent() and len(self["config"].getCurrent()) > 2 and self["config"].getCurrent()[2] or ""

	def createSummary(self):
		return SetupSummary

	def saveAll(self):
		for x in self["config"].list:
			x[1].save()

	def keyOk(self):
		if self["config"].getCurrent() and len(self["config"].getCurrent()) > 1 and self["config"].getCurrent()[1] == config.autobouquetsmaker.dayscreen:
			self.session.open(AutoBouquetsMakerDaysScreen)
		else:
			self.keySave()

	# keySave and keyCancel are just provided in case you need them.
	# you have to call them by yourself.
	def keySave(self):
		self.saveAll()
		self.close()

	def cancelConfirm(self, result):
		if not result:
			return
		for x in self["config"].list:
			x[1].cancel()
		self.close()

	def keyCancel(self):
		if self["config"].isChanged():
			self.session.openWithCallback(self.cancelConfirm, MessageBox, _("Really close without saving settings?"))
		else:
			self.close()

class AutoBouquetsMakerDaysScreen(ConfigListScreen, Screen):
	def __init__(self, session, args = 0):
		self.session = session
		Screen.__init__(self, session)
		Screen.setTitle(self, _('AutoBouquetsMaker') + " - " + _("Select days"))
		self.skinName = ["Setup"]
		self.config = config.autobouquetsmaker
		self.list = []
		days = (_("Monday"), _("Tuesday"), _("Wednesday"), _("Thursday"), _("Friday"), _("Saturday"), _("Sunday"))
		for i in sorted(self.config.days.keys()):
			self.list.append(getConfigListEntry(days[i], self.config.days[i]))
		ConfigListScreen.__init__(self, self.list)
		self["key_red"] = StaticText(_("Cancel"))
		self["key_green"] = StaticText(_("Save"))
		self["setupActions"] = ActionMap(["SetupActions", "ColorActions"],
		{
			"red": self.keyCancel,
			"green": self.keySave,
			"save": self.keySave,
			"cancel": self.keyCancel,
			"ok": self.keySave,
		}, -2)

	def keySave(self):
		if not any([self.config.days[i].value for i in self.config.days]):
			info = self.session.open(MessageBox, _("At least one day of the week must be selected"), MessageBox.TYPE_ERROR, timeout = 30)
			info.setTitle(_('Radio Times Emulator') + " - " + _("Select days"))
			return
		for x in self["config"].list:
			x[1].save()
		self.close()

	def keyCancel(self):
		if self["config"].isChanged():
			self.session.openWithCallback(self.cancelCallback, MessageBox, _("Really close without saving settings?"))
		else:
			self.cancelCallback(True)

	def cancelCallback(self, answer):
		if answer:
			for x in self["config"].list:
				x[1].cancel()
			self.close(False)


class SetupSummary(Screen):
	def __init__(self, session, parent):
		Screen.__init__(self, session, parent = parent)
		self["SetupTitle"] = StaticText(_(parent.setup_title))
		self["SetupEntry"] = StaticText("")
		self["SetupValue"] = StaticText("")
		self.onShow.append(self.addWatcher)
		self.onHide.append(self.removeWatcher)

	def addWatcher(self):
		self.parent.onChangedEntry.append(self.selectionChanged)
		self.parent["config"].onSelectionChanged.append(self.selectionChanged)
		self.selectionChanged()

	def removeWatcher(self):
		self.parent.onChangedEntry.remove(self.selectionChanged)
		self.parent["config"].onSelectionChanged.remove(self.selectionChanged)

	def selectionChanged(self):
		self["SetupEntry"].text = self.parent.getCurrentEntry()
		self["SetupValue"].text = self.parent.getCurrentValue()
		if hasattr(self.parent,"getCurrentDescription"):
			self.parent["description"].text = self.parent.getCurrentDescription()
