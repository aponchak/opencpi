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

//#define USE_EMULATOR

#ifdef NDEBUG
#undef NDEBUG
#endif

/*
 * Abstact:
 *  This file contains the implementation for a  PCI shared memory class
 *  implementation.
 *
 * Author: John Miller
 *
 * Date: 2/19/08
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/mman.h>
#include <time.h>
#include <CpiPCISMemServices.h>
#include <CpiUtilAutoMutex.h>
#include <CpiOsAssert.h>

using namespace DataTransfer;




void PCIInit()
{

}

// Create the service
void PCISmemServices::create (EndPoint* loc, CPI::OS::uint32_t size)
{
  CPI::Util::AutoMutex guard ( m_threadSafeMutex,
                               true ); 

  if ( m_init ) {
    return;
  }
  m_init = true;
  m_location = dynamic_cast<PCIEndPoint*>(loc);
  m_last_offset = 0;
  if ( size == 0 ) {
    size = m_location->map_size;
  }
  m_size = size;

  if ( loc->local ) {

#ifndef NDEBUG
    printf("************************ SMEM is local !!, vaddr = %p\n", m_vaddr);
#endif

    static const char *dma = getenv("CPI_DMA_MEMORY");
    CPI::OS::uint64_t base_adr;
    if (dma) {
      unsigned sizeM;
      cpiAssert(sscanf(dma, "%uM$0x%llx", &sizeM,
		       (unsigned long long *) &base_adr) == 2);
      size = (unsigned long long)sizeM * 1024 * 1024;
      fprintf(stderr, "DMA Memory:  %uM at 0x%llx\n", sizeM,
	      (unsigned long long)base_adr);
    }
    else {
      cpiAssert(!"CPI_DMA_MEMORY not found in the environment\n");
    }

    uint32_t offset = 1024*1024*128;
    size -= offset;
    base_adr+=offset;
    char buf[128];
    snprintf(buf, 128,
             "cpi-pci-pio://%s.%lld:%lld.2.10", "0", (unsigned long long)base_adr,
             (unsigned long long)size);
    loc->end_point = buf;

    if ( m_fd == -1 ) {
      if ( ( m_fd = open("/dev/mem", O_RDWR|O_SYNC )) < 0 ) {
	cpiAssert(0);
        throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "cant open /dev/mem"  );
      }
    }

    printf("mmap mapping base = %lld with size = %d\n", base_adr, size );

    m_vaddr =  (uint8_t*)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED,
                              m_fd, base_adr);


  }
  else {

    m_vaddr = NULL;

#ifndef NDEBUG
    printf("About to open shm\n");
#endif

    if ( m_fd == -1 ) {
      if ( (m_fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0) {
	cpiAssert(0);
        throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "cant open /dev/mem"  );
      }
    }

#ifndef NDEBUG
    printf("*******************  fd = %d *************************\n", m_fd);
#endif

  }
}


void* PCISmemServices::getHandle ()
{
  return NULL;
}

// Close shared memory object.
void PCISmemServices::close ()
{
#ifndef NDEBUG
  printf("In PCISmemServices::close ()\n");
#endif

}

// Attach to an existing shared memory object by name.
CPI::OS::int32_t PCISmemServices::attach (EndPoint*)
{
  return 0;
}

// Detach from shared memory object
CPI::OS::int32_t PCISmemServices::detach ()
{
  // NULL function
  return 0;
}


// Map a view of the shared memory area at some offset/size and return the virtual address.
void* PCISmemServices::map (CPI::OS::uint64_t offset, CPI::OS::uint32_t size )
{
  CPI::Util::AutoMutex guard ( m_threadSafeMutex,
                               true ); 

  if ( ! m_init  ) {
    create(m_location, m_location->size);
  }
        
  if ( m_location->local ) {        
#ifndef NDEBUG
    printf("**************** Returning local vaddr = %p\n", (uint8_t*)m_vaddr + offset);
#endif
    return (char*)m_vaddr + offset;
  }


  // We will create a map per offset until we change the CPI endpoint to handle segments
  // We just need to make sure we dont run out of maps.
  printf("shm size = %d, bus_offset = %llu, offset = %llu, fd = %d\n", 
         m_location->map_size, (long long)m_location->bus_offset,(long long)offset, m_fd );
  if (m_vaddr == NULL ) {
    m_vaddr = mmap(NULL, m_location->map_size, PROT_READ|PROT_WRITE, MAP_SHARED,
                    m_fd,  m_location->bus_offset );
    printf("vaddr = %p\n", m_vaddr );
  }

  if ( (m_vaddr == NULL) || (m_vaddr == (void*)-1 )) {
    printf("mmap failed to map to offset %llu\n", (long long)offset );
    throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "cant map offset"  );
  }
  return (CPI::OS::uint8_t*)m_vaddr+offset;
}

// Unmap the current mapped view.
CPI::OS::int32_t PCISmemServices::unMap ()
{
  return 0;
}

// Enable mapping
void* PCISmemServices::enable ()
{
  return m_vaddr;
}

// Disable mapping
CPI::OS::int32_t PCISmemServices::disable ()
{
  return 0;
}

PCISmemServices::~PCISmemServices ()
{

#ifndef NDEBUG
  printf("IN PCISmemServices::~PCISmemServices ()\n");
#endif

}


// Sets smem location data based upon the specified endpoint
CPI::OS::int32_t PCIEndPoint::setEndpoint( std::string& ep )
{
  EndPoint::setEndpoint(ep);

  printf("Scaning %s\n", ep.c_str() );
  if (sscanf(ep.c_str(), "cpi-pci-pio://%x.%lld:%d.3.10", &bus_id,
                   (long long unsigned *)&bus_offset,
                   (long long unsigned *)&map_size) != 3)
    throw CPI::Util::EmbeddedException("Remote endpoint description wrong: ");
  
#ifndef NDEBUG
  printf("bus_id = %d, offset = %llu, size = %d\n", bus_id, (long long)bus_offset, map_size );
#endif
   
  return 0;
}

PCIEndPoint::~PCIEndPoint(){}

