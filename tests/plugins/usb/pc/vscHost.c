#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <assert.h>

#define BUFFER_SIZE 1024 * 1024
#define PACKET_SIZE 512
#define DEV_INTERFACE 0
#define DEV_VID 0x040E
#define DEV_PID 0xf63B

int g_out_end_point = 0, g_in_end_point = 0;
#define VENDORID       0x040e
#define IS_TO_DEVICE   0   /* to device */
#define IS_TO_HOST     0x80   /* to host */
#define IS_BULK        2

// endpoint number is 1 based
#define FIRST_EP_NO 1
#define LAST_EP_NO 3

#define REPEAT_COUNT  500
#define TRANSFER_SIZE 1024*1024
#define TIMEOUT_MS    6000

unsigned char read_data[BUFFER_SIZE];
unsigned char write_data[BUFFER_SIZE];

unsigned int endpointNo;

pthread_t thread1, thread2;

libusb_device_handle *dev_handle=NULL;

int read_count = 0;
int write_count = 0;
//for md5

// leftrotate function definition
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))
 
// These vars will contain the hash
uint32_t h0, h1, h2, h3;
 
void md5(uint8_t *initial_msg, size_t initial_len) {
 
    // Message (to prepare)
    uint8_t *msg = NULL;
 
    // Note: All variables are unsigned 32 bit and wrap modulo 2^32 when calculating
 
    // r specifies the per-round shift amounts
 
    uint32_t r[] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                    5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
                    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

    // Use binary integer part of the sines of integers (in radians) as constants// Initialize variables:
    uint32_t k[] = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
        0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
        0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
        0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
        0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
        0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
        0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
        0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
        0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
        0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
        0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
        0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
        0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
        0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};
 
    h0 = 0x67452301;
    h1 = 0xefcdab89;
    h2 = 0x98badcfe;
    h3 = 0x10325476;
 
    // Pre-processing: adding a single 1 bit
    //append "1" bit to message    
    /* Notice: the input bytes are considered as bits strings,
       where the first bit is the most significant bit of the byte.[37] */
 
    // Pre-processing: padding with zeros
    //append "0" bit until message length in bit ≡ 448 (mod 512)
    //append length mod (2 pow 64) to message
 
    int new_len;
    for(new_len = initial_len*8 + 1; new_len%512!=448; new_len++);
    new_len /= 8;
 
    msg = calloc(new_len + 64, 1); // also appends "0" bits 
                                   // (we alloc also 64 extra bytes...)
    memcpy(msg, initial_msg, initial_len);
    msg[initial_len] = 128; // write the "1" bit
 
    uint32_t bits_len = 8*initial_len; // note, we append the len
    memcpy(msg + new_len, &bits_len, 4);           // in bits at the end of the buffer
 
    // Process the message in successive 512-bit chunks:
    //for each 512-bit chunk of message:
    int offset;
    for(offset=0; offset<new_len; offset += (512/8)) {
 
        // break chunk into sixteen 32-bit words w[j], 0 ≤ j ≤ 15
        uint32_t *w = (uint32_t *) (msg + offset);
 
#ifdef DEBUG
        printf("offset: %d %x\n", offset, offset);
 
        int j;
        for(j =0; j < 64; j++) printf("%x ", ((uint8_t *) w)[j]);
        puts("");
#endif
 
        // Initialize hash value for this chunk:
        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
 
        // Main loop:
        uint32_t i;
        for(i = 0; i<64; i++) {

#ifdef ROUNDS
            uint8_t *p;
            printf("%i: ", i);
            p=(uint8_t *)&a;
            printf("%2.2x%2.2x%2.2x%2.2x ", p[0], p[1], p[2], p[3], a);
         
            p=(uint8_t *)&b;
            printf("%2.2x%2.2x%2.2x%2.2x ", p[0], p[1], p[2], p[3], b);
         
            p=(uint8_t *)&c;
            printf("%2.2x%2.2x%2.2x%2.2x ", p[0], p[1], p[2], p[3], c);
         
            p=(uint8_t *)&d;
            printf("%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3], d);
            puts("");
#endif        

 
            uint32_t f, g;
 
             if (i < 16) {
                f = (b & c) | ((~b) & d);
                g = i;
            } else if (i < 32) {
                f = (d & b) | ((~d) & c);
                g = (5*i + 1) % 16;
            } else if (i < 48) {
                f = b ^ c ^ d;
                g = (3*i + 5) % 16;          
            } else {
                f = c ^ (b | (~d));
                g = (7*i) % 16;
            }

#ifdef ROUNDS
            printf("f=%x g=%d w[g]=%x\n", f, g, w[g]);
#endif 
            uint32_t temp = d;
            d = c;
            c = b;
            //printf("rotateLeft(%x + %x + %x + %x, %d)\n", a, f, k[i], w[g], r[i]);
            b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
            a = temp;


 
        }
 
        // Add this chunk's hash to result so far:
 
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
 
    }
 
    // cleanup
    free(msg);
 
}

static int GetEndPoint(libusb_device *dev) {
  struct libusb_config_descriptor *config;
  int r = libusb_get_active_config_descriptor(dev, &config);
  if (r < 0) {
    return -1;
  }

  int iface_idx;
  for (iface_idx = 0; iface_idx < config->bNumInterfaces; iface_idx++) {
    const struct libusb_interface *iface = &config->interface[iface_idx];
    int altsetting_idx;

    for (altsetting_idx = 0; altsetting_idx < iface->num_altsetting;
        altsetting_idx++) {
      const struct libusb_interface_descriptor *altsetting =
          &iface->altsetting[altsetting_idx];
      int ep_idx;

      for (ep_idx = 0; ep_idx < altsetting->bNumEndpoints; ep_idx++) {
        const struct libusb_endpoint_descriptor *ep =
            &altsetting->endpoint[ep_idx];

        if (IS_TO_DEVICE == (ep->bEndpointAddress & 0x80)
            && IS_BULK == (ep->bmAttributes & 0x03)) {
          g_out_end_point = ep->bEndpointAddress;
          printf("outEndPoint:[%x]\r\n", g_out_end_point);
        }
        if (IS_TO_HOST == (ep->bEndpointAddress & 0x80)
            && IS_BULK == (ep->bmAttributes & 0x03)) {
          g_in_end_point = ep->bEndpointAddress;
          printf("inEndPoint:[%x]\r\n", g_in_end_point);
        }
      }
    }

  }

  libusb_free_config_descriptor(config);

  return 0;
}

static void print_devs(libusb_device **devs,int* bus_id,int* device,uint8_t* rt_path,uint32_t* rt_bcd,int idx) {
  libusb_device *dev;
  int i = 0, j = 0;
  uint8_t path[8];
  int count= 0;

  while ((dev = devs[i++]) != NULL) {
    struct libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(dev, &desc);
    if (r < 0) {
      fprintf(stderr, "failed to get device descriptor");
      return;
    }
    if(desc.idVendor!=DEV_VID || desc.idProduct!=DEV_PID)
    {
      continue;
    }
    if(idx == count)
    {
      printf("%04X:%04X (bus %d, device %d, bcd 0x%x)", desc.idVendor, desc.idProduct,
        libusb_get_bus_number(dev), libusb_get_device_address(dev),desc.bcdUSB);
      *rt_bcd = desc.bcdUSB;
      *bus_id = libusb_get_bus_number(dev);
      *device = libusb_get_device_address(dev);
      r = libusb_get_port_numbers(dev, path, sizeof(path));
      if (r > 0) {
        memcpy(rt_path,path,sizeof(path));
        rt_path[8] = (uint8_t)r;
        printf(" path: %d", path[0]);
        for (j = 1; j < r; j++)
          printf(".%d", path[j]);
      }
      GetEndPoint(dev);
      printf("\n");
      return;

    }
    count++;


    
  }
}

static char s_pin_data1;
static char s_pin_data2;
static char s_pin_data3;
static char s_pin_data4;
static char s_pin_data5;

static uint32_t s_h0, s_h1, s_h2, s_h3;

void *readThread(void *args) {
  int r;
  int actual;
  int ep_no, k;
  static int first_time = 1;

  for (ep_no = FIRST_EP_NO; ep_no <= LAST_EP_NO; ep_no++) {
    for (k = 0; k < REPEAT_COUNT; k++) {
      r = libusb_bulk_transfer(dev_handle, (ep_no | LIBUSB_ENDPOINT_IN), read_data,
          TRANSFER_SIZE, &actual, TIMEOUT_MS);

      if (LIBUSB_ERROR_TIMEOUT == r) {
        r = 0;
        printf("read timeout\n");
        pthread_exit(0);
        break;
      } else if (r) {
        printf("Read Error: int_xfer=%d\n", r);
        pthread_exit(0);
        break;

      }
      if (actual > 0) {


        
        char pin_data1 = read_data[0];
        char pin_data2 = read_data[1];
        char pin_data3 = read_data[2];
        char pin_data4 = read_data[3];
        char pin_data5 = read_data[4];
        printf("\nDevice ID:%d%d%d%d%d\n",pin_data1,pin_data2,pin_data3,pin_data4,pin_data5);
        
        md5(read_data, actual);
        if(first_time == 1)
        {
          s_h0 = h0;
          s_h1 = h1;
          s_h2 = h2;
          s_h3 = h3;
          s_pin_data1 = pin_data1;
          s_pin_data2 = pin_data2;
          s_pin_data3 = pin_data3;
          s_pin_data4 = pin_data4;
          s_pin_data5 = pin_data5;
        }
        else
        {
          assert( s_h0 == h0);
          assert( s_h1 == h1);
          assert( s_h2 == h2);
          assert( s_h3 == h3);
          assert( s_pin_data1 == pin_data1);
          assert( s_pin_data2 == pin_data2);
          assert( s_pin_data3 == pin_data3);
          assert( s_pin_data4 == pin_data4);
          assert( s_pin_data5 == pin_data5);

        }


        uint8_t *p;
     
        // display result
        //it should be a6adf5ec 1a4e6623 2378a103 ccbd223e
        //a6adf5ec 1a4e6623 2378a103 ccbd223e
        //a6adf5ec 1a4e6623 2378a103 ccbd223e
        p=(uint8_t *)&h0;
        //printf("%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);
        /*
        assert( p[0] == 0xa6);
        assert( p[1] == 0xad);
        assert( p[2] == 0xf5);
        assert( p[3] == 0xec);
        */
        
        p=(uint8_t *)&h1;
        //printf("%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);
        /*
        assert( p[0] == 0x1a);
        assert( p[1] == 0x4e);
        assert( p[2] == 0x66);
        assert( p[3] == 0x23);
        */
        
        p=(uint8_t *)&h2;
        //printf("%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);
        /*
        assert( p[0] == 0x23);
        assert( p[1] == 0x78);
        assert( p[2] == 0xa1);
        assert( p[3] == 0x03);
        */
        
        p=(uint8_t *)&h3;
        //printf("%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);
        /*
        assert( p[0] == 0xcc);
        assert( p[1] == 0xbd);
        assert( p[2] == 0x22);
        assert( p[3] == 0x3e);
        */
        printf("[READ index:%d][ep %d] Read %d bytes, md5 check ok \n",read_count,ep_no,actual);
        //printf("Read %d bytes [ep %d]\n", actual, ep_no);
        //printf("expected a6adf5ec1a4e66232378a103ccbd223e\n");
        //printf("received %2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);
        read_count++;

        puts("");

      }
    }
  }

  pthread_exit(0);
}

void *writeThread(void *args) {
  int r;
  int actual;
  int k;
  int ep_no;

  for (ep_no = FIRST_EP_NO; ep_no <= LAST_EP_NO; ep_no++) {
    for(k = 0; k < REPEAT_COUNT; k ++) {
      r = libusb_bulk_transfer(dev_handle, (ep_no | LIBUSB_ENDPOINT_OUT), write_data,
          TRANSFER_SIZE, &actual, TIMEOUT_MS);

      if (LIBUSB_ERROR_TIMEOUT == r) {
        r = 0;
        printf("write timeout\n");
        pthread_exit(0);
        break;
      } else if (r) {
        printf("Write Error int_xfer=%d\n", r);
        pthread_exit(0);
        break;

      }
      if (actual > 0) {
        printf("[WRITE index: %d][ep %d] Write %d bytes ok \n",write_count,ep_no,actual);
        //printf("%d-----------Usb write------\n",write_count);
        //printf("\n\nWrote %d bytes [ep %d]\n", actual, ep_no);
        write_count++;
      }
    }
  }

  pthread_exit(0);
}

static void fillWriteBuffer(char *buff, int size)
{
  int i, j;

  assert(PACKET_SIZE != 0);
  assert(size % PACKET_SIZE == 0);
  for(i = 0; i < size / PACKET_SIZE; i++)
    for(j = 0; j < PACKET_SIZE; j++)
      buff[i*PACKET_SIZE + j] = i + 1;

}

int main(int argc, char** argv) {
 
  int idx = 0;
  if(argc>1)
    idx = atoi(argv[1]);
  libusb_device **devs;

  libusb_context *context = NULL;
  struct libusb_device_descriptor desc;
   
  int r;
  ssize_t cnt;

  pthread_attr_t attr;

  r = libusb_init(NULL);
  if (r < 0)
    return r;

  cnt = libusb_get_device_list(NULL, &devs);
  if (cnt < 0)
    return (int) cnt;

  uint8_t rt_path1[9];
  int bus_id1;
  int device1;
  int bcd1;
  uint8_t rt_path2[9];
  int bus_id2;
  int device2;
  int bcd2;
  print_devs(devs,&bus_id1,&device1,rt_path1,&bcd1,idx);
  int i = 0;
  int count=0;
  libusb_device *dev;
  int found = 0;
  while((dev = devs[i++]) != NULL)
  {
     
    if((r = libusb_get_device_descriptor(dev, &desc)) < 0)
    {

      printf( "Unable to get USB device descriptor: %s\n", libusb_strerror(r));
      continue;
    }
    //TODO: do we need to look for default_openvid when vid/pid == 0
    if( desc.idVendor == DEV_VID && desc.idProduct == DEV_PID)
    {
      if(idx == count)
      {
        found = 1;
        break;
      }
      count++;
    }
  }

  if(found == 0)
  {
    printf("not find device for idx: %d\n", idx);
    libusb_free_device_list(devs, 1);
    return -1;
  }
  r= libusb_open(dev, &dev_handle);
  if (r < 0) {
    printf("error code: %d\n", r);
    printf("Cannot open device\n");
    libusb_free_device_list(devs, 1);
    return 1;
  }

  //libusb_unref_device(devs[idx]);
  //libusb_detach_kernel_driver(dev_handle, 0);
  

  //dev_handle = libusb_open_device_with_vid_pid(context, DEV_VID, DEV_PID);

  if (dev_handle == NULL) {
    printf("Cannot open device\n");
     libusb_free_device_list(devs, 1);
    return 1;
  } else {
    printf("device opened busid: %d; devices: %d; \n",bus_id1,device1);
  }

  r = libusb_claim_interface(dev_handle, DEV_INTERFACE);
  if (r < 0) {
    printf("error code: %d\n", r);
    printf("Cannot claim interface\n");
    libusb_free_device_list(devs, 1);
    return 1;
  }
  fillWriteBuffer((char*)write_data,BUFFER_SIZE);
  /*
  md5(write_data,BUFFER_SIZE);
  uint8_t *p;
  
  p=(uint8_t *)&h0;
  printf("-----------write data MD5 CHECK------\n");
  printf("expected a6adf5ec1a4e66232378a103ccbd223e\n");
  printf("%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);

  assert( p[0] == 0xa6);
  assert( p[1] == 0xad);
  assert( p[2] == 0xf5);
  assert( p[3] == 0xec);
  
  p=(uint8_t *)&h1;
  printf("%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);
  assert( p[0] == 0x1a);
  assert( p[1] == 0x4e);
  assert( p[2] == 0x66);
  assert( p[3] == 0x23);
  
  p=(uint8_t *)&h2;
  printf("%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);
  assert( p[0] == 0x23);
  assert( p[1] == 0x78);
  assert( p[2] == 0xa1);
  assert( p[3] == 0x03);
  
  p=(uint8_t *)&h3;
  printf("%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);
  assert( p[0] == 0xcc);
  assert( p[1] == 0xbd);
  assert( p[2] == 0x22);
  assert( p[3] == 0x3e);
  puts("");
  */

  if (pthread_attr_init(&attr) != 0) {
    printf("pthread_attr_init error");
  }

  if (pthread_create( &thread1, &attr, &readThread,0) != 0) {
    printf("read thread creation failed\n");
    goto cleanup;
  }
  else
  {
    printf("Read thread created\n");
  }
  if (pthread_create( &thread2, &attr, &writeThread, 0) != 0) {
    printf("Write thread creation failed\n");
    goto cleanup;
  }
  else
  {
    printf("Write thread created\n");
  }

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

cleanup:
  print_devs(devs,&bus_id2,&device2,rt_path2,&bcd2,idx);
  //print_devs(devs,rt_path2);
  
  printf("----------------------------------------------------\n");
  printf("Usb test finished\n");
  printf("Read count is %d, expected is 1500\n",read_count);
  printf("Write count is %d,expected is 1500\n",write_count);

  printf("Before test usb info:\n\tbusid: %d;device: %d\n\tpath:%d\n\tBCD:0x%x\n", bus_id1, device1,rt_path1[0],bcd1);
  int j = 1;
  for (j = 1; j < rt_path1[8]; j++)
    printf(".%d", rt_path1[j]);
  printf("\n");
  printf("After test usb info:\n\tbusid: %d;device: %d\n\tpath:%d\n\tBCD:0x%x\n", bus_id2, device2,rt_path2[0],bcd2);
  //printf("After test the path is: %d\n", rt_path2[0]);
  //int j = 1;
  for (j = 1; j < rt_path2[8]; j++)
    printf(".%d", rt_path2[j]);
  printf("\n");
  if(read_count == 1500 && write_count == 1500)
    printf("test ok\n");
  else
    printf("test failed\n");


  r = libusb_release_interface(dev_handle, DEV_INTERFACE);
  if (r != 0) {
    printf("Cannot release interface\n");
    return 1;
  }
  libusb_close(dev_handle);
  libusb_free_device_list(devs, 1);
  libusb_exit(context);
  return 0;
}

