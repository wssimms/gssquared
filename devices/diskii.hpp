  
#define DiskII_Ph0_Off 0x00
#define DiskII_Ph0_On 0x01
#define DiskII_Ph1_Off 0x02
#define DiskII_Ph1_On 0x03
#define DiskII_Ph2_Off 0x04
#define DiskII_Ph2_On 0x05
#define DiskII_Ph3_Off 0x06
#define DiskII_Ph3_On 0x07
#define DiskII_Motor_Off 0x08
#define DiskII_Motor_On 0x09
#define DiskII_Drive1_Select 0x0A
#define DiskII_Drive2_Select 0x0B
#define DiskII_Q6L 0x0C
#define DiskII_Q6H 0x0D
#define DiskII_Q7L 0x0E
#define DiskII_Q7H 0x0F

void diskII_register_slot(cpu_state *cpu, uint8_t slot);
void mount_disk(uint8_t slot, uint8_t drive, const char *filename);
