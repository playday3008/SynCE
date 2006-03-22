/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to      *
 * permit persons to whom the Software is furnished to do so, subject to   *
 * the following conditions:                                               *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 ***************************************************************************/
#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

class DescriptorManager;


/**
 * @brief Representation of a network descriptor.
 *
 * This class represents an network descriptor and provides methods to manipulate the
 * behaviour of the socket. A descriptor represented by this class could be managed by
 * a DescriptorManager. It this managing DescriptorManager is one of the three
 * DescriptorManagers of the Multiplexer, the working method event() is automatically
 * called if an event has occured on the descriptor.
 *
 * @author Volker Christian
 */

class Descriptor{
public:
    /**
     * @brief Default constructor.
     *
     * This constructor creates an "empty" descriptor. This descriptor does not refer to
     * any open connection.
     */
    Descriptor();

    /**
     * @brief Copy constructor with the additional ability to release it from the DescriptorManager.
     *
     * This is the copy constructor. It has the additional ability to release the created
     * Descriptor from the DescriptorManager.
     * @param descriptor The source of the copy operation
     * @param releaseFromManager true, if the created descriptor should be released from
     *        the DescriptorManager, false otherwise.
     */
    Descriptor(const Descriptor &descriptor, bool releaseFromManager = false);

    /**
     * @brief Destructor.
     *
     * The destructor.
     */
    virtual ~Descriptor();

    /**
     * @brief Returns the associated DescriptorManager.
     * @return The associated DescriptorManager or NULL if none is assigned.
     */
    DescriptorManager *getDescriptorManager() const;

    /**
     * @brief Lesser-operator on the descriptor base.
     *
     * A network descriptor is represented as an integer value by the underlying
     * Operating System. This operator works at that level and compares Descriptors by
     * their integer values.
     * @param descriptor The right hand side of the lesser-operator.
     * @return true, if the left hand side is smaller than the right hand size. False
     * otherwise.
     */
    bool operator<(Descriptor &descriptor) {
        if (this->descriptor < descriptor.getDescriptor()) {
            return true;
        }
        return false;
    }

    /**
     * @brief The assignment-operator.
     * @param descriptor The right hand size of the assigment-operator.
     * @return The assigned Descriptor.
     */
    Descriptor &operator=(const Descriptor &descriptor) {
        this->descriptor = descriptor.descriptor;
        this->descriptorManager = descriptor.descriptorManager;
        return *this;
    }

    /**
     * @brief Returns the integer value which represents the descriptor by the underlying
     * Operating System.
     * @return The integer value representing the descriptor.
     */
    int getDescriptor() const;

    /**
     * @brief Closes the descriptor.
     * @return True if success, false on error.
     */
    virtual bool close();

    /**
     * @brief Check if data are pending to read.
     * @param sec Seconds to wait for data
     * @param usec Microseconds to wait for data
     * @return True, if data are pending false otherwise.
     */
    bool dataPending(int sec, int usec);

    /**
     * @brief Set the descriptor to non blocking mode.
     * @return True on success, falso on error.
     */
    bool setNonBlocking();

protected:
    virtual void event() = 0;
    void setDescriptor(int descriptor);

private:
    static bool sortcrit(const Descriptor *d1, const Descriptor *d2);
    bool setDescriptorManager(DescriptorManager *descriptorManager);

private:
    DescriptorManager *descriptorManager;
    int descriptor;

friend class DescriptorManager;
};

#endif
