#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_FILENAME_LENGTH 1000
#define MAX_COMMIT_MESSAGE_LENGTH 2000
#define MAX_LINE_LENGTH 1000
#define MAX_MESSAGE_LENGTH 1000

int neoGitExists()
{
    char cwd[1024];
    if(getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Cannot get current directory");
        return 1;    
    }

    struct dirent *entry;
    char cwd2[1024];
    bool exists = false;

    do{
        DIR *dir = opendir(".");
        if(dir == NULL){
            perror("Cannot open current directory");
            return 1;
        }
        while((entry = readdir(dir)) != NULL){
            if(entry->d_type == DT_DIR && strcmp(entry->d_name, ".neogit") == 0){
                exists = true;
                break;
            }    
        }
        closedir(dir);

        if (getcwd(cwd2, sizeof(cwd2)) == NULL) return 1;

        if (strcmp(cwd2, "/") != 0){
            if (chdir("..") != 0) return 1;
        }
    }while (strcmp(cwd2, "/") != 0);
    
    if (chdir(cwd) != 0) return 1;

    if(exists)
        return 0;
    else
        return 1;
}

int globalConfig(int argc , char *argv[]){
    FILE *fp = fopen("globalConfig", "a+");

    if(! strcmp(argv[3], "user.name")) fprintf(fp, "username: %s\n", argv[4]);
    else if(! strcmp(argv[3], "user.email")) fprintf(fp, "email: %s\n", argv[4]);
    else {
        perror("invalid command");
        return 1;
    }
    fclose(fp);
    if(neoGitExists() == 0){
        FILE *fp2 = fopen(".neogit/config", "w");
        if (fp2 == NULL) {
            perror("Cannot open file!");
            return 1;
        }

        if(! strcmp(argv[3], "user.name")) fprintf(fp2, "username: %s\n", argv[4]);
        else if(! strcmp(argv[3], "email.name")) fprintf(fp2, "email: %s\n", argv[4]);
        fclose(fp2);
    }
    printf("global configuration set successfully!");
    return 0;
}

int localConfig(int argc , char *argv[]){
    if(neoGitExists() == 0){
        FILE *fp = fopen(".neogit/config", "w");
        if (fp == NULL) {
            perror("Cannot open file!");
            return 1;
        }

        if(! strcmp(argv[3], "user.name")) fprintf(fp, "username: %s\n", argv[4]);
        else if(! strcmp(argv[3], "email.name")) fprintf(fp, "email: %s\n", argv[4]);
        fclose(fp); 
    }
    else{
        perror("neogit repository has not been initialized yet!");
        return 1;
    }
    fprintf(stdout, "local configuration set successfully!");
    return 0;
}

int createConfigs(char *username, char *email) {
    FILE *fp = fopen(".neogit/config", "w");
    if (fp == NULL) return 1;

    fprintf(fp, "username: %s\n", username);
    fprintf(fp, "email: %s\n", email);
    fprintf(fp, "last_commit_ID: %d\n", 0);
    fprintf(fp, "current_commit_ID: %d\n", 0);
    fprintf(fp, "branch: %s", "master");

    fclose(fp);
    
    if (mkdir(".neogit/commits") != 0) return 1;
    if (mkdir(".neogit/files") != 0) return 1;

    fp = fopen(".neogit/staging", "w");
    fclose(fp);

    fp = fopen(".neogit/tracks", "w");
    fclose(fp);

    return 0;
}

int add(int argc, char *const argv[]) {
    if (argc < 3) {
        perror("please specify a file");
        return 1;
    }
    FILE *fp = fopen(".neogit/staging", "r");
    if (fp == NULL) return 1;
    if(!strcmp(argv[2], "-f")){
        for(int i = 3; i < argc; i++)
        {
            if(strchr(argv[i], '*') != NULL)
                addToStage(argv[i], 'w');
            else
                addToStage(argv[i], 'n');
        }
    }
    else if(!strcmp(argv[2], "-n")){
        run_add_n();
        return 0;
    }
    else if(strchr(argv[2], '*') != NULL){
        return addToStage(argv[2], 'w');
    }
    else{
        return addToStage(argv[2], 'n');
    }
    return 0;
}

int addToStage(char *filepath , char mode)
{
    FILE *stagingfile = fopen(".neogit/staging", "a+");
    if(stagingfile == NULL) {
        perror("Error while getting the staging file");
        return 1;
    }
    char line[MAX_LINE_LENGTH];
    while(fgets(line, sizeof(line), stagingfile) != NULL)
    {
        int length = strlen(line);

        if (length > 0 && line[length - 1] == '\n') {
            line[length - 1] = '\0';
        }
        if(mode == 'n')
            if (strcmp(filepath, line) == 0){
                fclose(stagingfile);
                return 0;
            } 
        else if(mode == 'w')
            if (fnmatch(filepath, line) == 0){
                fclose(stagingfile);
                return 0;
            } 
    }
    fclose(stagingfile);

    stagingfile = fopen(".neogit/staging", "a");
    if (stagingfile == NULL) {
        perror("Error opening staging file");
        return 1;
    }
    if(mode == 'n')
        if(!file_exists(filepath)){
            perror("file does not exist");
            fclose(stagingfile);
            return 1;
        }
        else{
            struct stat path_stat;
            stat(filepath, &path_stat);

            if (S_ISDIR(path_stat.st_mode)) {
                add_directory_to_staging(filepath);
            } 
            else {
                fprintf(stagingfile, "%s\n", filepath);
            }
        }
            
    else{
        DIR *dir = opendir(".");
        if (dir == NULL) {
            perror("Error opening current directory");
            fclose(stagingfile);
            return 1;
        }
        struct dirent *entry;
        bool found = false;
        while ((entry = readdir(dir)) != NULL) {
            if (fnmatch(filepath, entry->d_name) == 0) {
                fprintf(stagingfile, "%s\n", entry->d_name);
                found = true;
            }
        }
        if(! found){
            perror("file does not exist");
            fclose(stagingfile);
            closedir(dir);
            return 1;
        }
        closedir(dir);
    }
    fclose(stagingfile);
    fprintf(stdout, "adds to staging area\n");
    return 0;
}

int reset(int argc, char *const argv[]) {
    if (argc < 3) {
        perror("please specify a file");
        return 1;
    }
    
    return remove_from_staging(argv[2]);
}



int main(int argc, char **argv){
    if (argc < 2) {
        fprintf(stdout, "Invalid command!");
        return 0;
    }
    if(!strcmp(argv[1],"config")){
        if(! strcmp(argv[2], "-global")){
            if(! strcmp(argv[3], "user.name") || ! strcmp(argv[3], "user.email"))
                return globalConfig(argc , argv);
        }
        else if(! strcmp(argv[2], "user.name") || ! strcmp(argv[2], "user.email"))
            return localConfig(argc , argv);
        else
            fprintf(stdout, "Invalid command!");
    }
    if (!strcmp(argv[1], "init"))
        return init(argc, argv);
    if (!strcmp(argv[1], "add"))
        return add(argc, argv);
    if (!strcmp(argv[1], "reset"))
        return reset(argc, argv);
}