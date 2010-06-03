// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

/*
 * Abstact:
 *   This file contains the Interface for the Cpi port meta data class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef CPI_DataTransport_PortMetaData_H_
#define CPI_DataTransport_PortMetaData_H_

#include <CpiTransportConstants.h>
#include <DtOsDataTypes.h>
#include <CpiRDTInterface.h>
#include <CpiParentChild.h>
namespace CU = ::CPI::Util;

namespace DataTransfer {
  class XferFactory;
  struct EndPoint;
}

namespace CPI {

  namespace DataTransport {

    // Forward references
    struct PortSetMetaData;
    class PortSet;

    // Port Ordinal
    typedef CPI::OS::int32_t PortOrdinal;

    struct PortMetaData : public CPI::Util::Child<PortSetMetaData,PortMetaData>
    {

      // Are we a shadow port >
      bool m_shadow;

      // Location of the real port
      std::string real_location_string;

      // Our location if we are a shadow port
      std::string shadow_location_string;

      // Remote Circuit id if we are a shadow
      CPI::OS::int32_t       remoteCircuitId;
      CPI::OS::int64_t   remotePortId;

      // port id
      PortOrdinal id;

      // Our rank in the port set
      CPI::OS::uint32_t rank;

      // Output port
      bool output;

      // user data
      void* user_data;

      // Our port set metadata
      PortSetMetaData* m_portSetMd;

      // Data initialized
      bool m_init;

      // Our location, host, etc.
      DataTransfer::EndPoint    * m_real_location;
      DataTransfer::XferFactory * m_real_tfactory;
      DataTransfer::EndPoint    * m_shadow_location;
      DataTransfer::XferFactory * m_shadow_tfactory;

      // Ports local "port set" control structure offset
      DtOsDataTypes::Offset m_localPortSetControl;

      // Offsets for port communication structures
      struct OutputPortBufferControlMap {
        DtOsDataTypes::Offset bufferOffset;                  // offset to our buffer
        CPI::OS::uint32_t bufferSize;                          // Buffer size in bytes
        DtOsDataTypes::Offset localStateOffset;          // offset to our state structure
        DtOsDataTypes::Offset metaDataOffset;                  // offset to our meta-data structure
        DtOsDataTypes::Offset portSetControlOffset;  // offset to our port set control structure
      };

      // In this structure, the number of offsets for the remote state and meta data
      // is equal to the number of ports in the output port set.
      struct InputPortBufferControlMap {
        DtOsDataTypes::Offset bufferOffset;                            // offset to our buffer
        CPI::OS::uint32_t bufferSize;                                    // Buffer size in bytes

        /*
         *  The input buffers need N number of local states where N is the number of 
         *  output ports that can write to the input.  We will create a contiguous array
         *  of states so we only need 1 offset
         */
        DtOsDataTypes::Offset localStateOffset; // offset to our state structure

        /*
         *  The remote state structure contains the offsets to all of our remote
         *  states.  The remote states exist in the "shadow" input buffers in our
         *  represented port for each circuit instance that exists.  The number of 
         *  circuit instances that exist is equal to the number of output ports * 
         *  the number of input ports, however the only ones that we need to be concerned
         *  with are the ones that exist in circuits that have "real" output ports.
         *
         *  If this is a shadow port, these are not initialized.
         */

        // Offsets to our remote "shadow" input ports states.  When we indicate 
        // that this buffer is empty, we need to inform all of the shadows that have
        // a "real" output port.   This array is indexed by the port id, so only the output
        // port id's are valid.
        DtOsDataTypes::Offset myShadowsRemoteStateOffsets[MAX_PCONTRIBS]; 


        // Each output that can write data to our buffer will also write its meta-data here.
        // This array is also indexed by the output port id.  We will create a contiguous array
        // so we only need 1 offset
        DtOsDataTypes::Offset metaDataOffset;
        CPI::OS::uint32_t     metaDataSize;

      };

      union BufferOffsets {
        struct OutputPortBufferControlMap outputOffsets;
        struct InputPortBufferControlMap inputOffsets;
      };

      // Our local "real" descriptor
      CPI::RDT::Descriptors m_descriptor;

      // This is our shadow ports descriptor that we will pass to allow external ports to connect to us.
      CPI::RDT::Descriptors m_shadowPortDescriptor;

      // This is the descriptor that we get from an external port that we are attempting to connect to.
      CPI::RDT::Descriptors m_externPortDependencyData;

      struct CpiPortDependencyData  {
        BufferOffsets *offsets;  // buffer offsets
      };

      // Here is our buffer offset information.  This is an array that is "N" buffers deep
      BufferOffsets *m_bufferData;

      // Standard constructors
      PortMetaData( PortOrdinal pid, 
                    bool s, 
                    CPI::RDT::Descriptors& sPort,
                    const char * shadow_ep,
                    PortSetMetaData* psmd );

      PortMetaData( PortOrdinal pid, 
                    bool output,
                    const char* ep, 
                    const char * shadow_ep,
                    PortSetMetaData* psmd );



      PortMetaData( PortOrdinal pid, 
                    CPI::RDT::Descriptors& portDesc,
                    PortSetMetaData* psmd );

      // Dependency constructor
      PortMetaData( PortOrdinal pid, 
                    bool s, 
                    const char* ep, 
                    const char* shadow_ep,
                    CPI::RDT::Descriptors& pd, 
                    CPI::OS::uint32_t circuitId,
                    PortSetMetaData* psmd );

      virtual ~PortMetaData();

      /**********************************
       * Common init
       **********************************/
      void init();

    };

    /**********************************
     ****
     * inline declarations
     ****
     *********************************/

  }

}

#endif

