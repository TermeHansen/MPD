/*
 * Copyright 2003-2017 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include "Device.hxx"
#include "Util.hxx"
#include "lib/expat/ExpatParser.hxx"

#include <string.h>

UPnPDevice::~UPnPDevice()
{
	/* this destructor exists here just so it won't get inlined */
}

/**
 * An XML parser which constructs an UPnP device object from the
 * device descriptor.
 */
class UPnPDeviceParser final : public CommonExpatParser {
	UPnPDevice &m_device;

	std::string *value;

	UPnPService m_tservice;

public:
	UPnPDeviceParser(UPnPDevice& device)
		:m_device(device),
		 value(nullptr) {}

protected:
	virtual void StartElement(const XML_Char *name, const XML_Char **) {
		value = nullptr;

		switch (name[0]) {
		case 'c':
			if (strcmp(name, "controlURL") == 0)
				value = &m_tservice.controlURL;
			break;
		case 'd':
			if (strcmp(name, "deviceType") == 0)
				value = &m_device.deviceType;
			break;
		case 'f':
			if (strcmp(name, "friendlyName") == 0)
				value = &m_device.friendlyName;
			break;
		case 'm':
			if (strcmp(name, "manufacturer") == 0)
				value = &m_device.manufacturer;
			else if (strcmp(name, "modelName") == 0)
				value = &m_device.modelName;
			break;
		case 's':
			if (strcmp(name, "serviceType") == 0)
				value = &m_tservice.serviceType;
			break;
		case 'U':
			if (strcmp(name, "UDN") == 0)
				value = &m_device.UDN;
			else if (strcmp(name, "URLBase") == 0)
				value = &m_device.URLBase;
			break;
		}
	}

	virtual void EndElement(const XML_Char *name) {
		if (value != nullptr) {
			trimstring(*value);
			value = nullptr;
		} else if (!strcmp(name, "service")) {
			m_device.services.emplace_back(std::move(m_tservice));
			m_tservice.clear();
		}
	}

	virtual void CharacterData(const XML_Char *s, int len) {
		if (value != nullptr)
			value->append(s, len);
	}
};

void
UPnPDevice::Parse(const std::string &url, const char *description)
{
	{
		UPnPDeviceParser mparser(*this);
		mparser.Parse(description, strlen(description), true);
	}

	if (URLBase.empty()) {
		// The standard says that if the URLBase value is empty, we should use
		// the url the description was retrieved from. However this is
		// sometimes something like http://host/desc.xml, sometimes something
		// like http://host/

		if (url.size() < 8) {
			// ???
			URLBase = url;
		} else {
			auto hostslash = url.find_first_of("/", 7);
			if (hostslash == std::string::npos || hostslash == url.size()-1) {
				URLBase = url;
			} else {
				URLBase = path_getfather(url);
			}
		}
	}
}
