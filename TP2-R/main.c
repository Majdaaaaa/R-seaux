//
//  main.c
//  TP2-R
//
//  Created by Majda Benmalek on 06/02/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/syslimits.h>
#include <string.h>
#include <unistd.h>

int main(int argc, const char * argv[]) {
    
    // le res nous montre que la machine est en little-endian
    uint32_t var = 0xba21;
    printf("%x\n",var);
    uint32_t res = htonl(var);
    printf("%x\n",res);
    printf("sizeof(int): %lu\n",sizeof(int));
    printf("sizeof(uint32_t): %lu\n",sizeof(uint32_t));
    
    uint32_t witness = 0x01020304;
    unsigned char *bytes = (unsigned char*)&witness;
    
    printf("Ordre des octets en mémoire :\n");
    for (size_t i = 0; i < sizeof(witness); i++) {
        printf("%02x ", bytes[i]);
    }
    printf("\n");
    return 0;
    
}
