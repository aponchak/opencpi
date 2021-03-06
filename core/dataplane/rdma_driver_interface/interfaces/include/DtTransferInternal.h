
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


/**
   @file

   @brief
   This file contains the Interface for all SMB transfer types.

   Revision History:

   3/1/2005 - John Miller
   Initial version.

   2/18/2009 - John Miller
   Removed exception monitor class.

************************************************************************** */

#ifndef DataTransfer_TransferInternal_H_
#define DataTransfer_TransferInternal_H_

#include <string.h>
#include <list>
#include <OcpiOsMutex.h>
#include <OcpiList.h>
//#include <DtHandshakeControl.h>
#include <DtExceptions.h>
#include <OcpiDriverManager.h>
#include <DtTransferInterface.h>
#include <DtSharedMemoryInternal.h>


namespace DataTransfer {

  // Forward references
  class SmemServices;


  // This is the transfer factory manager
  // - the driver manager for transfer drivers
  extern const char *transfer;
  class XferFactoryManager : 
    public OCPI::Driver::ManagerBase<XferFactoryManager, XferFactory, transfer>,
    public FactoryConfig
  {
  public:
    friend class XferFactory;
    inline static XferFactoryManager& getFactoryManager() {
      return getSingleton();
    }
    inline size_t getSMBSize() { return m_SMBSize; }

    // This method is used to retreive all of the available endpoints that have been
    // registered in the system.  Note: some of the endpoints may not be finalized. 
    void allocateSupportedEndpoints(EndPoints &endpoints);
    std::vector<std::string> getListOfSupportedEndpoints();  
    std::vector<std::string> getListOfSupportedProtocols();  

    // This method allocates an endpoint based upon the requested protocol.  The size is the
    // requested size of the memory block to associate with the endpoint, this call is responsible
    // for setting the actual size that was allocated.
    std::string allocateEndpoint(std::string& protocol, const OCPI::Util::PValue *props=NULL);

    // Configure the manager and it drivers
    void configure(  ezxml_t config );

    // Create a transfer compatible SMB and associated resources
    SMBResources* createSMBResources(EndPoint* loc);

    // Delete a SMB resource
    //    void deleteSMBResources(EndPoint* loc);

    // get the transfer compatible SMB resources
#if 1
    SMBResources* getSMBResources( std::string& endpoint ); 
    inline SMBResources* getSMBResources( const char* ep )
      {std::string t(ep); return getSMBResources(t);}
#endif
    SMBResources* getSMBResources( EndPoint* ep );

    // Retrieves the factory based upon the transfer type
    static std::string null;
    XferFactory* find( std::string& end_point1,std::string& end_point2 = null );
    XferFactory* find( const char* end_point1,const char* end_point2 = NULL);

    // Creates a transfer service template
    //    XferServices* getService(std::string& s_endpoint,  std::string& t_endpoint);        
    XferServices* getService(EndPoint *s_endpoint, EndPoint *t_endpoint);        

    // Create an endpoint for remote hardware
    EndPoint& allocateProxyEndPoint(const char *epString, size_t size);

    static bool canSupport(EndPoint &local_ep, const char *remote_endpoint);
    // Constructors/Destructors
    XferFactoryManager();
    ~XferFactoryManager();

  protected:

    // Clears all cached data
    void clearCache();

    // Starts up the transfer sub-system
    void startup();

    // Shuts down the transer sub-system
    void shutdown();

    int get_template(EndPoint *src, EndPoint *dst, XferServices* &xfer_template);
    int add_template(EndPoint *src, EndPoint *dst, XferServices* xf_template);
    //    SMBResources* findResource(const char* );
    
    // Reference counter
    OCPI::OS::uint32_t m_refCount;
    bool               m_init;
    OCPI::OS::Mutex    m_mutex;
    //    OCPI::Util::VList  m_resources;
    List               m_templatelist;
    FactoryConfig      m_config;

    bool            m_configured;

  };

    // Convenience
    inline XferFactoryManager &getManager() { return XferFactoryManager::getSingleton(); }


}

#endif

