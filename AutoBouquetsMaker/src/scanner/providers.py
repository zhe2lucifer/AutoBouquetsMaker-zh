from .. import log
import os
import xml.dom.minidom
import cPickle as pickle
from enigma import eDVBFrontendParametersSatellite, eDVBFrontendParametersTerrestrial, eDVBFrontendParametersCable

class Providers():
	VALID_PROTOCOLS = ( "fastscan", "freesat", "lcn", "lcn2", "lcnbat", "lcnbat2", "nolcn", "sky", "vmuk", "vmuk2" )
	PROVIDERS_DIR = os.path.dirname(__file__) + "/../providers"

	def parseXML(self, filename):
		try:
			provider = open(filename, "r")
		except Exception, e:
			print>>log, "[ABM-Providers][parseXML] Cannot open %s: %s" % (filename, e)
			return None

		try:
			dom = xml.dom.minidom.parse(provider)
		except Exception, e:
			print>>log, "[ABM-Providers][parseXML] XML parse error (%s): %s" % (filename, e)
			provider.close()
			return None

		provider.close()
		return dom

	def providerFileExists(self, name):
		providers_dir = self.PROVIDERS_DIR
		filename = name + ".xml"
		return os.path.exists(providers_dir + "/" + filename)

	def read(self):
		providers_dir = self.PROVIDERS_DIR
		cachefile = "providers.cache" # cache file
		providers = {}

		# check if providers cache exists and data is fresh
		newest = 0
		for filename in os.listdir(providers_dir):
			if filename[-4:] != ".xml":
				continue
			filetime = os.path.getmtime(providers_dir + "/" + filename)
			if filetime > newest:
				newest = filetime
		try:
			if os.path.exists(providers_dir + "/" + cachefile) and os.path.getmtime(providers_dir + "/" + cachefile) > newest:
				with open(providers_dir + "/" + cachefile, 'rb') as cache_input:
					providers = pickle.load(cache_input)
					cache_input.close()
					return providers
		except:
			pass

		# cache file does not exist or data is stale
		for filename in os.listdir(providers_dir):
			if filename[-4:] != ".xml":
				continue

			dom = self.parseXML(providers_dir + "/" + filename)
			if dom is None:
				continue

			provider = {}
			provider["key"] = filename[:-4]
			provider["swapchannels"] = []
			provider["dependent"] = ''
			provider["bouquets"] = {}
			provider["ignore_visible_service_flag"] = 0
			if dom.documentElement.nodeType == dom.documentElement.ELEMENT_NODE and dom.documentElement.tagName == "provider":
				for node in dom.documentElement.childNodes:
					if node.nodeType != node.ELEMENT_NODE:
						continue

					if node.tagName == "name":
						node.normalize()
						if len(node.childNodes) == 1 and node.childNodes[0].nodeType == node.TEXT_NODE:
							provider["name"] = node.childNodes[0].data.encode("utf-8")
					elif node.tagName == "streamtype":
						node.normalize()
						if len(node.childNodes) == 1 and node.childNodes[0].nodeType == node.TEXT_NODE:
							provider["streamtype"] = node.childNodes[0].data.encode("utf-8")
					elif node.tagName == "protocol":
						node.normalize()
						if len(node.childNodes) == 1 and node.childNodes[0].nodeType == node.TEXT_NODE and node.childNodes[0].data.encode("utf-8") in self.VALID_PROTOCOLS:
							provider["protocol"] = node.childNodes[0].data.encode("utf-8")
					elif node.tagName == "transponder":
						transponder = {}
						transponder["nit_pid"] = 0x10
						transponder["nit_current_table_id"] = 0x40
						transponder["nit_other_table_id"] = 0x41
						transponder["sdt_pid"] = 0x11
						transponder["sdt_current_table_id"] = 0x42
						transponder["sdt_other_table_id"] = 0x46
						transponder["bat_pid"] = 0x11
						transponder["bat_table_id"] = 0x4a
						transponder["fastscan_pid"] = 0x00			# no default value
						transponder["fastscan_table_id"] = 0x00		# no default value
						transponder["system"] = eDVBFrontendParametersSatellite.System_DVB_S
						transponder["polarization"] = eDVBFrontendParametersSatellite.Polarisation_Horizontal
						transponder["fec_inner"] = eDVBFrontendParametersSatellite.FEC_Auto
						transponder["modulation"] = eDVBFrontendParametersSatellite.Modulation_QPSK
						transponder["inversion"] = eDVBFrontendParametersSatellite.Inversion_Unknown
						transponder["roll_off"] = eDVBFrontendParametersSatellite.RollOff_alpha_0_35
						transponder["pilot"] = eDVBFrontendParametersSatellite.Pilot_Unknown
						transponder["onid"] = None
						transponder["tsid"] = None
						for i in range(0, node.attributes.length):
							if node.attributes.item(i).name == "frequency":
								transponder["frequency"] = int(node.attributes.item(i).value)
							elif node.attributes.item(i).name == "symbol_rate":
								transponder["symbol_rate"] = int(node.attributes.item(i).value)
							elif node.attributes.item(i).name == "polarization":
								transponder["polarization"] = int(node.attributes.item(i).value)
							elif node.attributes.item(i).name == "fec_inner":
								transponder["fec_inner"] = int(node.attributes.item(i).value)
							elif node.attributes.item(i).name == "orbital_position":
								transponder["orbital_position"] = int(node.attributes.item(i).value)
							elif node.attributes.item(i).name == "inversion":
								transponder["inversion"] = int(node.attributes.item(i).value)
							elif node.attributes.item(i).name == "system":
								transponder["system"] = int(node.attributes.item(i).value)
							elif node.attributes.item(i).name == "modulation":
								transponder["modulation"] = int(node.attributes.item(i).value)
							elif node.attributes.item(i).name == "roll_off":
								transponder["roll_off"] = int(node.attributes.item(i).value)
							elif node.attributes.item(i).name == "pilot":
								transponder["pilot"] = int(node.attributes.item(i).value)
#							elif node.attributes.item(i).name == "bandwidth":
#								transponder["bandwidth"] = int(node.attributes.item(i).value)
#							elif node.attributes.item(i).name == "code_rate_hp":
#								transponder["code_rate_hp"] = int(node.attributes.item(i).value)
#							elif node.attributes.item(i).name == "code_rate_lp":
#								transponder["code_rate_lp"] = int(node.attributes.item(i).value)
#							elif node.attributes.item(i).name == "transmission_mode":
#								transponder["transmission_mode"] = int(node.attributes.item(i).value)
#							elif node.attributes.item(i).name == "guard_interval":
#								transponder["guard_interval"] = int(node.attributes.item(i).value)
#							elif node.attributes.item(i).name == "hierarchy":
#								transponder["hierarchy"] = int(node.attributes.item(i).value)
							elif node.attributes.item(i).name == "nit_pid":
								transponder["nit_pid"] = int(node.attributes.item(i).value, 16)
							elif node.attributes.item(i).name == "nit_current_table_id":
								transponder["nit_current_table_id"] = int(node.attributes.item(i).value, 16)
							elif node.attributes.item(i).name == "nit_other_table_id":
								transponder["nit_other_table_id"] = int(node.attributes.item(i).value, 16)
							elif node.attributes.item(i).name == "sdt_pid":
								transponder["sdt_pid"] = int(node.attributes.item(i).value, 16)
							elif node.attributes.item(i).name == "sdt_current_table_id":
								transponder["sdt_current_table_id"] = int(node.attributes.item(i).value, 16)
							elif node.attributes.item(i).name == "sdt_other_table_id":
								transponder["sdt_other_table_id"] = int(node.attributes.item(i).value, 16)
							elif node.attributes.item(i).name == "bat_pid":
								transponder["bat_pid"] = int(node.attributes.item(i).value, 16)
							elif node.attributes.item(i).name == "bat_table_id":
								transponder["bat_table_id"] = int(node.attributes.item(i).value, 16)
							elif node.attributes.item(i).name == "fastscan_pid":
								transponder["fastscan_pid"] = int(node.attributes.item(i).value, 16)
							elif node.attributes.item(i).name == "fastscan_table_id":
								transponder["fastscan_table_id"] = int(node.attributes.item(i).value, 16)
							elif node.attributes.item(i).name == "onid":
								transponder["onid"] = int(node.attributes.item(i).value)
							elif node.attributes.item(i).name == "tsid":
								transponder["tsid"] = int(node.attributes.item(i).value)

						if len(transponder.keys()) in (22, 18):
							provider["transponder"] = transponder

					elif node.tagName == "bouquettype":
						node.normalize()
						if len(node.childNodes) == 1 and node.childNodes[0].nodeType == node.TEXT_NODE:
							provider["bouquettype"] = node.childNodes[0].data.encode("utf-8")

					elif node.tagName == "netid":
						node.normalize()
						if len(node.childNodes) == 1 and node.childNodes[0].nodeType == node.TEXT_NODE:
							provider["netid"] = node.childNodes[0].data.encode("utf-8")

					elif node.tagName == "dvbsconfigs":
						for node2 in node.childNodes:
							if node2.nodeType == node2.ELEMENT_NODE and node2.tagName == "configuration":
								configuration = {}
								for i in range(0, node2.attributes.length):
									if node2.attributes.item(i).name == "key":
										configuration["key"] = node2.attributes.item(i).value.encode("utf-8")
									elif node2.attributes.item(i).name == "bouquet":
										configuration["bouquet"] = int(node2.attributes.item(i).value, 16)
									elif node2.attributes.item(i).name == "region":
										configuration["region"] = int(node2.attributes.item(i).value, 16)

								node2.normalize()
								if len(node2.childNodes) == 1 and node2.childNodes[0].nodeType == node2.TEXT_NODE:
									configuration["name"] = node2.childNodes[0].data.encode("utf-8")

								if len(configuration.keys()) == 4:
									provider["bouquets"][configuration["key"]] = configuration

					elif node.tagName == "dvbcconfigs":
						transponder = {}
						transponder["nit_pid"] = 0x10
						transponder["nit_current_table_id"] = 0x40
						transponder["nit_other_table_id"] = 0x41
						transponder["sdt_pid"] = 0x11
						transponder["sdt_current_table_id"] = 0x42
						transponder["sdt_other_table_id"] = 0x46
						transponder["bat_pid"] = 0x11
						transponder["bat_table_id"] = 0x4a
						for node2 in node.childNodes:
							if node2.nodeType == node2.ELEMENT_NODE and node2.tagName == "configuration":
								configuration = {}
								configuration["fec_inner"] = eDVBFrontendParametersCable.FEC_Auto
								configuration["inversion"] = eDVBFrontendParametersCable.Inversion_Unknown
								configuration["modulation"] = eDVBFrontendParametersCable.Modulation_Auto
								configuration["onid"] = None
								configuration["tsid"] = None
								for i in range(0, node2.attributes.length):
									if node2.attributes.item(i).name == "key":
										configuration["key"] = node2.attributes.item(i).value.encode("utf-8")
									elif node2.attributes.item(i).name == "netid":
										configuration["netid"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "bouquettype":
										configuration["bouquettype"] = node2.attributes.item(i).value.encode("utf-8")
									elif node2.attributes.item(i).name == "frequency":
										configuration["frequency"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "symbol_rate":
										configuration["symbol_rate"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "fec_inner":
										configuration["fec_inner"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "inversion":
										configuration["inversion"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "system":
										configuration["system"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "modulation":
										configuration["modulation"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "bouquet":
										configuration["bouquet"] = int(node2.attributes.item(i).value, 16)
									elif node2.attributes.item(i).name == "region":
										configuration["region"] = int(node2.attributes.item(i).value, 16)
									elif node2.attributes.item(i).name == "onid":
										configuration["onid"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "tsid":
										configuration["tsid"] = int(node2.attributes.item(i).value)

								node2.normalize()
								if len(node2.childNodes) == 1 and node2.childNodes[0].nodeType == node2.TEXT_NODE:
									configuration["name"] = node2.childNodes[0].data.encode("utf-8")

								if len(configuration.keys()) == 12 and 'lcnbat' not in provider["protocol"] and 'region' not in configuration and 'bouquet' not in configuration:
									provider["bouquets"][configuration["key"]] = configuration
								elif len(configuration.keys()) == 14 and 'lcnbat' in provider["protocol"]:
									provider["bouquets"][configuration["key"]] = configuration

						if len(transponder.keys()) == 8:
							provider["transponder"] = transponder

					elif node.tagName == "dvbtconfigs":
						transponder = {}
						transponder["nit_pid"] = 0x10
						transponder["nit_current_table_id"] = 0x40
						transponder["nit_other_table_id"] = 0x00
						transponder["sdt_pid"] = 0x11
						transponder["sdt_current_table_id"] = 0x42
						transponder["sdt_other_table_id"] = 0x46
						transponder["bat_pid"] = 0x11
						transponder["bat_table_id"] = 0x4a

						for node2 in node.childNodes:
							if node2.nodeType == node2.ELEMENT_NODE and node2.tagName == "configuration":
								configuration = {}
								configuration["system"] = eDVBFrontendParametersTerrestrial.System_DVB_T
								configuration["inversion"] = eDVBFrontendParametersTerrestrial.Inversion_Unknown
								configuration["modulation"] = eDVBFrontendParametersTerrestrial.Modulation_Auto
								configuration["bandwidth"] = 8000000
								configuration["code_rate_hp"] = eDVBFrontendParametersTerrestrial.FEC_Auto
								configuration["code_rate_lp"] = eDVBFrontendParametersTerrestrial.FEC_Auto
								configuration["transmission_mode"] = eDVBFrontendParametersTerrestrial.TransmissionMode_Auto
								configuration["guard_interval"] = eDVBFrontendParametersTerrestrial.GuardInterval_Auto
								configuration["hierarchy"] = eDVBFrontendParametersTerrestrial.Hierarchy_Auto
								configuration["onid"] = None
								configuration["tsid"] = None

								for i in range(0, node2.attributes.length):
									if node2.attributes.item(i).name == "key":
										configuration["key"] = node2.attributes.item(i).value.encode("utf-8")
									elif node2.attributes.item(i).name == "frequency":
										configuration["frequency"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "inversion":
										configuration["inversion"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "modulation":
										configuration["modulation"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "system":
										configuration["system"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "bandwidth":
										configuration["bandwidth"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "code_rate_hp":
										configuration["code_rate_hp"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "code_rate_lp":
										configuration["code_rate_lp"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "transmission_mode":
										configuration["transmission_mode"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "guard_interval":
										configuration["guard_interval"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "hierarchy":
										configuration["hierarchy"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "onid":
										configuration["onid"] = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "tsid":
										configuration["tsid"] = int(node2.attributes.item(i).value)

								node2.normalize()
								if len(node2.childNodes) == 1 and node2.childNodes[0].nodeType == node2.TEXT_NODE:
									configuration["name"] = node2.childNodes[0].data.encode("utf-8")

								if len(configuration.keys()) == 14:
									provider["bouquets"][configuration["key"]] = configuration

						if len(transponder.keys()) == 8:
							provider["transponder"] = transponder

					elif node.tagName == "sections":
						provider["sections"] = {}
						for node2 in node.childNodes:
							if node2.nodeType == node2.ELEMENT_NODE and node2.tagName == "section":
								number = -1
								for i in range(0, node2.attributes.length):
									if node2.attributes.item(i).name == "number":
										number = int(node2.attributes.item(i).value)

								if number == -1:
									continue

								node2.normalize()
								if len(node2.childNodes) == 1 and node2.childNodes[0].nodeType == node2.TEXT_NODE:
									provider["sections"][number] = node2.childNodes[0].data.encode("utf-8")

					elif node.tagName == "swapchannels":
						for node2 in node.childNodes:
							if node2.nodeType == node2.ELEMENT_NODE and node2.tagName == "channel":
								channel_number = -1
								channel_with = -1
								channel_conditional = None
								for i in range(0, node2.attributes.length):
									if node2.attributes.item(i).name == "number":
										channel_number = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "with":
										channel_with = int(node2.attributes.item(i).value)
									elif node2.attributes.item(i).name == "conditional": # allows adding an evalable condition on a channel by channel basis
										channel_conditional = node2.attributes.item(i).value.encode("utf-8")

								if channel_number != -1 and channel_with != -1:
									if channel_conditional is None:
										provider["swapchannels"].append([channel_number, channel_with])
									else:
										provider["swapchannels"].append([channel_number, channel_with, channel_conditional])

					elif node.tagName == "servicehacks":
						node.normalize()
						for i in range(0, len(node.childNodes)):
							if node.childNodes[i].nodeType == node.CDATA_SECTION_NODE:
								provider["servicehacks"] = node.childNodes[i].data.encode("utf-8").strip()

					elif node.tagName == "dependent":
						node.normalize()
						if len(node.childNodes) == 1 and node.childNodes[0].nodeType == node.TEXT_NODE:
							provider["dependent"] = node.childNodes[0].data.encode("utf-8")

					elif node.tagName == "visibleserviceflag":
						for i in range(0, node.attributes.length):
							if node.attributes.item(i).name == "ignore" and int(node.attributes.item(i).value) != 0:
								provider["ignore_visible_service_flag"] = 1

			if not ("name" in provider
					and "protocol" in provider
					and "streamtype" in provider
					and "bouquets" in provider
					and "sections" in provider
					and "transponder" in provider
					and "servicehacks" in provider):

				print>>log, "[ABM-Providers][read] Incomplete XML %s" % filename
				continue

			providers[provider["key"]] = provider
		try:
			with open(providers_dir + "/" + cachefile, 'wb') as cache_output:
				pickle.dump(providers, cache_output, pickle.HIGHEST_PROTOCOL)
				cache_output.close()
		except:
			pass
		return providers
