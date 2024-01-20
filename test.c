#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#define MAX_NUMBER_OF_WORDS 100
#define PATH "/usr/share/dict"
//"../usr/share/dict/words"

char *removeSpace(char *str);


int main(int argc, char **argv) {

    // char buffer[250];
    // char newBuffer[250];
    // strcpy(buffer, "hello how are you   ");
    // strcpy(newBuffer, removeSpace(buffer));
    // printf("buffer = %sx\n", buffer);
    // printf("newBuffer = %sx\n", newBuffer);

    DIR *dirp = opendir(PATH);
    struct dirent *entryName;
    FILE *file;
    int index = 0;
    int arraySize = MAX_NUMBER_OF_WORDS;
    char filePath[512];
    char line[256];
    char **toReturn = NULL;
    if (dirp == NULL) {
        perror("Error opening the directory");
        return 1;
    }
   
    while((entryName = readdir(dirp)) != NULL) {
        if (entryName->d_type == DT_REG) { // check if its a file
            snprintf(filePath, sizeof(filePath), "%s/%s", PATH, entryName->d_name);
            printf("entirePath = %s\n", filePath);
            file = fopen(filePath, "r");
            if(file == NULL) {
                perror("Error opening the file");
                continue;
            }
            toReturn = (char **)malloc(MAX_NUMBER_OF_WORDS * sizeof(char*));
            printf("Reaching till here, before fgets\n");
            while (fscanf(file, "%256s", line) == 1) {
                if (index == arraySize) {
                    arraySize = arraySize * 2;
                    toReturn = realloc(toReturn, arraySize * sizeof(char *));
                    if (toReturn == NULL) {
                        perror("Memroy reallocation failed");
                        exit(EXIT_FAILURE);
                    }
                }
                toReturn[index] = strdup(line);
                index++;
            }
            fclose(file);
        }
    }   
    closedir(dirp);
    for (size_t i = 0; i < index; i++) {
        printf("i = %zu, word = %s\n", i, toReturn[i]);
    }
    for (size_t i = 0; i < index; i++) {
        free(toReturn[i]);
    }
    free(toReturn);
}

char *removeSpace(char *str) {
    int lengthOfString = strlen(str);
    int j = 0;
    char *toReturn = (char *)malloc(lengthOfString + 1);
    
    for(int i = 0; i < lengthOfString; i++) {
        if (!isspace((unsigned char)str[i])) {
            toReturn[j] = str[i];
            j++;
        }
    }
    toReturn[j] = '\0';
    return toReturn;
}
