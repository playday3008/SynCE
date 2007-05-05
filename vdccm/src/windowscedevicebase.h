//
// C++ Interface: windowscedevicebase
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef WINDOWSCEDEVICEBASE_H
#define WINDOWSCEDEVICEBASE_H

#include <synce.h>
#include <string>

using namespace std;

/**
	@author Volker Christian <voc@users.sourceforge.net>
*/

class WindowsCEDeviceBase{
public:

    bool operator==(const WindowsCEDeviceBase &device) const {
        return this->getDeviceName() == device.getDeviceName();
    }

    class EqualsTo : public std::unary_function<WindowsCEDeviceBase *, bool> {
        public:
            EqualsTo(const WindowsCEDeviceBase *device) : device(device) {}
            bool operator()(const WindowsCEDeviceBase *device) {
                return (*this->device == *device);
            }
            const WindowsCEDeviceBase *device;
    };

    virtual ~WindowsCEDeviceBase(){};
/*!
    \fn WindowsCEDevice::isLocked() const
 */
    virtual bool isLocked() const = 0;


/*!
    \fn WindowsCEDevice::getKey() const
 */
    virtual int getKey() const = 0;

/*!
    \fn WindowsCEDevice::getDeviceAddress() const
 */
    virtual string getDeviceAddress() const = 0;


/*!
    \fn WindowsCEDevice::getDeviceClass() const
 */
    virtual string getDeviceClass() const = 0;

/*!
    \fn WindowsCEDevice::getDeviceName() const
 */
    virtual string getDeviceName() const = 0;

/*!
    \fn WindowsCEDevice::getRealName() const
 */
    virtual string getRealName() const = 0;


/*!
    \fn WindowsCEDevice::getHardware() const
 */
    virtual string getHardware() const = 0;

/*!
    \fn WindowsCEDevice::getPassword() const
 */
    virtual string getPassword() const = 0;

/*!
    \fn WindowsCEDevice::getBuildNumber() const
 */
    virtual uint32_t getBuildNumber() const = 0;


/*!
    \fn WindowsCEDevice::getOsVersion() const
 */
    virtual uint32_t getOsVersion() const = 0;


/*!
    \fn WindowsCEDevice::getProcessorType() const
 */
    virtual uint32_t getProcessorType() const = 0;


/*!
    \fn WindowsCEDevice::getPartnerId1() const
 */
    virtual uint32_t getPartnerId1() const = 0;


/*!
    \fn WindowsCEDevice::getPartnerId2() const
 */
    virtual uint32_t getPartnerId2() const = 0;

/*!
    \fn WindowsCEDevice::getPort() const
 */
    virtual uint32_t getPort() const = 0;


    virtual void disconnect() = 0;


    virtual void ping() = 0;

    virtual string getTransport() const = 0;

    virtual bool sendPassword(string password)
    {
        return true;
    }
};

#endif
