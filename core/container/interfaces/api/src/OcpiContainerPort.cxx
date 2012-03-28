
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "OcpiContainerPort.h"
#include "OcpiWorker.h"
#include "OcpiContainerApplication.h"
#include "OcpiContainerMisc.h"
#include "OcpiPValue.h"

namespace OCPI {
  namespace Container {
    namespace OA = OCPI::API;
    namespace OM = OCPI::Metadata;
    namespace OU = OCPI::Util;

    PortData::PortData(const OM::Port &mPort, bool isProvider, unsigned xferOptions,
		       const OU::PValue *params, PortConnectionDesc *desc)
      : m_isProvider(isProvider), m_connectionData(desc)
    {
      OCPI::RDT::Descriptors &d = getData().data;
      d.type = isProvider ? OCPI::RDT::ConsumerDescT : OCPI::RDT::ProducerDescT;
      d.role = OCPI::RDT::NoRole;
      d.options = xferOptions;
      bzero((void *)&d.desc, sizeof(d.desc));
      d.desc.nBuffers = DEFAULT_NBUFFERS > mPort.minBufferCount ? DEFAULT_NBUFFERS : mPort.minBufferCount;
      if (!(d.desc.dataBufferSize = mPort.bufferSize))
	d.desc.dataBufferSize = DEFAULT_BUFFER_SIZE;
      setPortParams(mPort, params);
    }
    // Set parameters for a port, whether at creation/construction time or later at connect time
    void PortData::setPortParams(const OM::Port &mPort, const OU::PValue *params) {
      OA::ULong ul;

      if (OU::findULong(params, "bufferCount", ul))
	if (ul < mPort.minBufferCount)
	  throw ApiError("bufferCount is below worker's minimum", NULL);
        else
	  getData().data.desc.nBuffers = ul;
      if (OU::findULong(params, "bufferSize", ul))
	if (ul < mPort.m_minBufferSize)
	  throw ApiError("bufferSize is below worker's minimum", NULL);
        else
	  getData().data.desc.dataBufferSize = ul;
      const char *s;
      if (OU::findString(params, "xferRole", s)) {
	OCPI::RDT::PortRole role;
	if (!strcasecmp(s, "passive"))
	  role = OCPI::RDT::Passive;
	else if (!strcasecmp(s, "active") ||
		 !strcasecmp(s, "activemessage"))
	  role = OCPI::RDT::ActiveMessage;
	else if (!strcasecmp(s, "flowcontrol") ||
		 !strcasecmp(s, "activeflowcontrol"))
	  role = OCPI::RDT::ActiveFlowControl;
	else if (!strcasecmp(s, "activeonly"))
	  role = OCPI::RDT::ActiveOnly;
	else
	  throw ApiError("xferRole property must be passive|active|flowcontrol|activeonly", NULL);
	if (!(getData().data.options & (1 << role)))
	  throw ApiError("xferRole of \"%s\" not supported by port \"%s\"", s, mPort.name);
	getData().data.role = role;
      }
    }

    BasicPort::BasicPort(const OCPI::Metadata::Port & metaData, bool isProvider, unsigned options,
			 const OU::PValue *params, PortConnectionDesc *desc)
      : PortData(metaData, isProvider, options, params, desc), myDesc(getData().data.desc), m_metaPort(metaData)
    {
    }

    BasicPort::~BasicPort(){}

    // This base class constructor for generic initialization
    // FIXME: parse buffer count here at least? (check that others don't do it).
    Port::Port(Container &container, const OCPI::Metadata::Port &mPort, bool isProvider,
	       unsigned xferOptions, const OCPI::Util::PValue *params, PortConnectionDesc *desc) :
      BasicPort( mPort, isProvider, xferOptions, params, desc),
      m_container(container),
      m_canBeExternal(true)
    {
      // getData().container_id = m_container.getId();  
    }

    Container &Port::container() const { return m_container; }

    void Port::loopback(OA::Port &) {}

    bool Port::hasName(const char *name) {
      return (name == m_metaPort.name );
    }

    // The default behavior is that there is nothing special to do between
    // ports of like containers.
    bool Port::connectLike(Port &other, const OCPI::Util::PValue *myProps,
			   const OCPI::Util::PValue *otherProps) {
      (void)other;(void)myProps;(void)otherProps;
      return false;
    }

    // The general case of connecting ports that are in the same process.
    void Port::connect(OCPI::API::Port &apiOther, const OCPI::Util::PValue *myParams,
		       const OCPI::Util::PValue *otherParams) {
      Port &other = *static_cast<Port*>(&apiOther);
      setMode( CON_TYPE_RDMA );
      other.setMode( CON_TYPE_RDMA );
      if (isProvider())
        if (other.isProvider())
          throw ApiError("Cannot connect two provider ports", NULL);
        else
          other.connect( *this, otherParams, myParams);
      else if (!other.isProvider())
        throw ApiError("Cannot connect to user ports", NULL);
      else {
        Container &otherContainer = other.container();
        // Containers know how to do internal connections
        if (&m_container == &otherContainer) {
	  other.applyConnectParams(otherParams);
	  applyConnectParams(myParams);
          connectInside(other, myParams, otherParams);
	}
        // Container MAY know how to do intercontainer connections between like containers.
        else if (&container().driver() == &otherContainer.driver() &&
                 connectLike( other, myParams, otherParams))
	  return;
        else {
	  // We're in different containers managed locally.  Perform the connection protocol
          // FIXME:  a more "binary" way to do this locally?
	  std::string pInfo, uInfo;
	  
	  // if output has a single protocol, tell the input about it.
	  const char *dummy;
	  OU::PValue *others = NULL;
	  if (!OU::findString(otherParams, "protocol", dummy) &&
	      !OU::findString(otherParams, "transport", dummy) &&
	      !OU::findString(otherParams, "endpoint", dummy) &&
	      getPreferredProtocol(dummy)) {
	    // FIXME: make a convenience method for PVList etc.
	    others = new OU::PValue[otherParams->length() + 2];
	    unsigned n = 0;
	    const OU::PValue *p;
	    for (p = otherParams; p && p->name; p++, n++)
	      others[n] = *p;
	    others[n].name = "protocol";
	    others[n].vString = dummy;
	    others[n].type = OA::OCPI_String;
	    others[++n].name = NULL;
	    otherParams = others;
	  }
	  other.getInitialProviderInfo(otherParams, pInfo);
	  if (others)
	    delete [] others;
	  setInitialProviderInfo(myParams, pInfo, uInfo);
          if (!uInfo.empty()) {
            other.setInitialUserInfo(uInfo, pInfo);
            if (!pInfo.empty()) {
	      setFinalProviderInfo(pInfo, uInfo);
              if (!uInfo.empty())
                other.setFinalUserInfo(uInfo);
            }
	  }
        }
      }
    }


    // The general case of connecting ports that are in the same process.
    void Port::connectURL(const char*, const OCPI::Util::PValue *,
			  const OCPI::Util::PValue *) {
      printf("connectURL not allowed on this container !!\n");
      ocpiAssert( 0 );
    }


    // Start the remote/intercontainer connection process
    // FIXME: Need simpler protocol to connect between containers in same process
    // without all this pack/unpack overhead.
    // We do not set up the OCDP here since we don't know everything.
    void Port::getInitialProviderInfo(const OCPI::Util::PValue *params, std::string &out) {
      ocpiAssert(isProvider());
      if (!m_canBeExternal)
	throw OU::Error("Port ", m_metaPort.name, " cannot be connected external to container");
      applyConnectParams(params);
      m_container.packPortDesc(this->getData(), out);
    }

    // User side initial method, that carries provider info and returns user info
    void Port::setInitialProviderInfo(const OCPI::Util::PValue *params,
				       const std::string &ipi, std::string &out) {
      // User side, producer side.
      ocpiAssert(!isProvider());
      if (!m_canBeExternal)
	throw ApiError("Port \", name, \" cannot be connected external to container", NULL);
      PortConnectionDesc otherPortData;
      m_container.unpackPortDesc(ipi, otherPortData);
      // Adjust any parameters from connection metadata
      applyConnectParams(params);
      // We now know the role aspects of both sides.  Make the decision so we know what
      // resource allocations to make in finishConnection.
      determineRoles(otherPortData.data);
      finishConnection(otherPortData.data);
      // We're done but other side still needs info.
      // FIXME done here?
      m_container.packPortDesc(this->getData(), out);
    }

    void Port::setInitialUserInfo(const std::string &iui, std::string &out) {
      ocpiAssert(isProvider());
      PortConnectionDesc otherPortData;
      m_container.unpackPortDesc(iui, otherPortData);
      determineRoles(otherPortData.data);
      // Adjust any parameters from connection metadata
      finishConnection(otherPortData.data);
      out.clear();
    }

    // User only
    void Port::setFinalProviderInfo(const std::string &, std::string &out) {
      out.clear();
    }
    // Provider Only
    void Port::setFinalUserInfo(const std::string &) {
    }
    // Establish the roles, which might happen earlier than the finalization of the connection
    // Since roles can determine resource allocations
    // This could be table-driven...
    void Port::determineRoles(OCPI::RDT::Descriptors &other) {
      static const char *roleName[] =
	{"ActiveMessage", "ActiveFlowControl", "ActiveOnly", "Passive", "MaxRole", "NoRole"};

      OCPI::RDT::Descriptors
        &pDesc = isProvider() ? getData().data : other,
        &uDesc = isProvider() ? other : getData().data;
      printf("Port %s, a %s, has options 0x%x, initial role %s, buffers %u\n"
             "  other has options 0x%x, initial role %s, buffers %u\n",
             m_metaPort.name, isProvider() ? "provider/consumer" : "user/producer",
             getData().data.options, roleName[getData().data.role], getData().data.desc.nBuffers,
             other.options, roleName[other.role], other.desc.nBuffers);
      chooseRoles(uDesc.role, uDesc.options, pDesc.role, pDesc.options);
      printf("  after negotiation, port %s, a %s, has role %s\n"
             "  other has role %s\n",
             m_metaPort.name, isProvider() ? "provider/consumer" : "user/producer",
             roleName[getData().data.role], roleName[other.role]);
      // We must make sure other side doesn't mess with roles anymore.
      uDesc.options |= 1 << OCPI::RDT::MandatedRole;
      pDesc.options |= 1 << OCPI::RDT::MandatedRole;
    }
    // Convert PValues into descriptor values, with metadata constraint checking

    void BasicPort::setConnectParams(const OCPI::Util::PValue *params) {
      setPortParams(m_metaPort, params);
      // There are no connection parameters (yet) other than those that can be provided
      // to ports before connection.
    }
    // Default = we don't check or do anything here as generic parameters are set.
    void BasicPort::checkConnectParams() {}
    // Do the work on this port when connection properties are specified.
    // This is still prior to receiving info from the other side, thus this is not necessarily
    // the final info.
    // FIXME: we should error check against bitstream-fixed parameters
    void BasicPort::applyConnectParams(const OCPI::Util::PValue *params) {
      setConnectParams(params);
      checkConnectParams();
    }
    namespace {
      void defaultRole(int32_t &role, uint32_t options) {
	if (role == OCPI::RDT::NoRole) {
	  for (unsigned n = 0; n < OCPI::RDT::MaxRole; n++)
	    if (options & (1 << n)) {
	      role = n;
	      return;
	    }
	  throw OU::Error("Container port has no transfer roles");
	}
      }
    }

    // coming in, specified roles are preferences or explicit instructions.
    // The existing settings are either NoRole, a preference, or a mandate
    void BasicPort::chooseRoles(int32_t &uRole, uint32_t uOptions, int32_t &pRole, uint32_t pOptions) {
      // FIXME this relies on knowledge of the values of the enum constants
      static OCPI::RDT::PortRole otherRoles[] =
        {OCPI::RDT::ActiveFlowControl, OCPI::RDT::ActiveMessage,
         OCPI::RDT::Passive, OCPI::RDT::ActiveOnly};
      defaultRole(uRole, uOptions);
      defaultRole(pRole, pOptions);
      OCPI::RDT::PortRole
        pOther = otherRoles[pRole],
        uOther = otherRoles[uRole];
      if (pOptions & (1 << OCPI::RDT::MandatedRole)) {
        // provider has a mandate
        ocpiAssert(pRole != OCPI::RDT::NoRole);
        if (uRole == pOther)
          return;
        if (uOptions & (1 << OCPI::RDT::MandatedRole))
          throw ApiError("Incompatible mandated transfer roles", NULL);
        if (uOptions & (1 << pOther)) {
          uRole = pOther;
          return;
        }
        throw ApiError("No compatible role available against mandated role", NULL);
      } else if (pRole != OCPI::RDT::NoRole) {
        // provider has a preference
        if (uOptions & (1 << OCPI::RDT::MandatedRole)) {
          // user has a mandate
          ocpiAssert(uRole != OCPI::RDT::NoRole);
          if (pRole == uOther)
            return;
          if (pOptions & (1 << uOther)) {
            pRole = uOther;
            return;
          }
          throw ApiError("No compatible role available against mandated role", NULL);
        } else if (uRole != OCPI::RDT::NoRole) {
          // We have preferences on both sides, but no mandate
          // If preferences match, all is well
          if (pRole == uOther)
            return;
          // If one preference is against push, we better listen to it.
          if (uRole == OCPI::RDT::ActiveFlowControl &&
              pOptions & (1 << OCPI::RDT::ActiveMessage)) {
            pRole = OCPI::RDT::ActiveMessage;
            return;
          }
          // Let's try active push if we can
          if (uRole == OCPI::RDT::ActiveMessage &&
              pOptions & (1 << OCPI::RDT::ActiveFlowControl)) {
            pRole = OCPI::RDT::ActiveFlowControl;
            return;
          }
          if (pRole == OCPI::RDT::ActiveFlowControl &&
              uOptions & (1 << OCPI::RDT::ActiveMessage)) {
            uRole = OCPI::RDT::ActiveFlowControl;
            return;
          }
          // Let's try activeonly push if we can
          if (uRole == OCPI::RDT::ActiveOnly &&
              pOptions & (1 << OCPI::RDT::Passive)) {
            pRole = OCPI::RDT::Passive;
            return;
          }
          if (pRole == OCPI::RDT::Passive &&
              pOptions & (1 << OCPI::RDT::ActiveOnly)) {
            pRole = OCPI::RDT::ActiveOnly;
            return;
          }
          // Let's give priority to the "better" role.
          if (uRole < pRole &&
              pOptions & (1 << uOther)) {
            pRole = uOther;
            return;
          }
          // Give priority to the provider
          if (uOptions & (1 << pOther)) {
            uRole = pOther;
            return;
          }
          if (pOptions & (1 << uOther)) {
            pRole = uOther;
            return;
          }
          // Can't use either preference.  Fall throught to no mandates, no preferences
        } else {
          // User role unspecified, but provider has a preference
          if (uOptions & (1 << pOther)) {
            uRole = pOther;
            return;
          }
          // Can't use provider preference, Fall through to no mandates, no preferences
        }
      } else if (uOptions & (1 << OCPI::RDT::MandatedRole)) {
        // Provider has no mandate or preference, but user has a mandate
        if (pOptions & (1 << uOther)) {
          pRole = uOther;
          return;
        }
        throw ApiError("No compatible role available against mandated role", NULL);
      } else if (uRole != OCPI::RDT::NoRole) {
        // Provider has no mandate or preference, but user has a preference
        if (pOptions & (1 << uOther)) {
          pRole = uOther;
          return;
        }
        // Fall through to no mandates, no preferences.
      }
      // Neither has useful mandates or preferences.  Find anything, biasing to push
      for (int i = 0; i < OCPI::RDT::MaxRole; i++)
        // Provider has no mandate or preference
        if (uOptions & (1 << i) &&
            pOptions & (1 << otherRoles[i])) {
          uRole = i;
          pRole = otherRoles[i];
          return;
        }
      throw ApiError("No compatible combination of roles exist", NULL);
    }            
    ExternalPort::ExternalPort(const OCPI::Metadata::Port & metaPort,
			       bool isProvider, const OCPI::Util::PValue *params)
      : BasicPort(metaPort, isProvider, 0, params) {
    }
    void ExternalPort::checkConnectParams() {}
  }
  namespace API {
    ExternalBuffer::~ExternalBuffer(){}
    ExternalPort::~ExternalPort(){}
    Port::~Port(){}
  }
}
