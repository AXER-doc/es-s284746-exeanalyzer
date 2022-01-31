#include <stdio.h>
#include <stdlib.h>
#include <winnt.h>

void skip(FILE *file, size_t bytes) {
    for (int i = 0; i < bytes; ++i) fgetc(file);
}
WORD reverse_read_2bytes(FILE *file) {
    BYTE byte1, byte2;
    byte1 = fgetc(file);
    byte2 = fgetc(file);
    return byte1 + byte2 * 256;
}
DWORD reverse_read_4bytes(FILE *file) {
    BYTE *bytes = malloc(4);
    fread(bytes, 1, 4, file);
    DWORD res = 0;
    for (int i = 0; i < 4; ++i) {
        res *= 256;
        res += bytes[3-i];
    }
    free(bytes);
    return res;
}
struct DATA {
    DWORD e_lfanew;
    WORD NumberOfSections;
    WORD SizeOfOptionalHeader;
    DWORD AddressOfEntryPoint;
    IMAGE_SECTION_HEADER *section;
};
char* dword_to_hex(DWORD data) {
    char *res = malloc(11);
    res[0] = '0';
    res[1] = 'x';
    for (short i = 0; i < 8; ++i) {
        short n = data % 16;
        char ch = (n < 10) ? (char)(n + 48) : (char)(n + 55);
        res[9-i] = ch;
        data /= 16;
    }
    res[10] = '\0';
    return res;
}
short len_of_num(int n) {
    if (n < 0) return 0;
    if (n == 0) return 1;
    short k = 0;
    while (n > 0) {
        ++k;
        n /= 10;
    }
    return k;
}
char* int_to_string(int n) {
    short s = len_of_num(n);
    char *res = malloc(s+1);
    for (short i = 1; i <= s; ++i) {
        res[s-i] = (char)(n % 10 + 48);
        n /= 10;
    }
    res[s] = '\0';
    return res;
}
void write_int(int n, FILE *file) {
    char *number = int_to_string(n);
    fwrite(number, 1, len_of_num(n), file);
    free(number);
}
void write_dword(DWORD data, FILE *file) {
    char *str = dword_to_hex(data);
    fwrite(str, 1, 10, file);
    free(str);
}
void print_data(struct DATA data, FILE *file) {
    fwrite("e_lfanew: ", 1, 10, file);
    write_dword(data.e_lfanew, file);
    fwrite("\nNumberOfSections: ", 1, 19, file);
    write_int(data.NumberOfSections, file);
    fwrite("\nSizeOfOptionalHeader: ", 1, 23, file);
    write_int(data.SizeOfOptionalHeader, file);
    fwrite("\nAddressOfEntryPoint: ", 1, 22, file);
    write_dword(data.AddressOfEntryPoint, file);
}
void print_section(struct DATA d, int i, FILE *f) {
    fwrite("Section ", 1, 8, f);
    write_int(i+1, f);
    fwrite(" {\n\tName: ", 1, 10, f);
    fwrite(d.section[i].Name, 1, 8, f);
    fwrite("\n\tPhysicalAddress: ", 1, 19, f);
    write_dword(d.section[i].Misc.PhysicalAddress, f);
    fwrite("\n\tVirtualAddress: ", 1, 18, f);
    write_dword(d.section[i].VirtualAddress, f);
    fwrite("\n\tSizeOfRawData: ", 1, 17, f);
    write_dword(d.section[i].SizeOfRawData, f);
    fwrite("\n\tPointerToRawData: ", 1, 20, f);
    write_dword(d.section[i].PointerToRawData, f);
    fwrite("\n\tPointerToRelocations: ", 1, 24, f);
    write_dword(d.section[i].PointerToRelocations, f);
    fwrite("\n\tPointerToLinenumbers: ", 1, 24, f);
    write_dword(d.section[i].PointerToLinenumbers, f);
    fwrite("\n\tNumberOfRelocations: ", 1, 23, f);
    write_int(d.section[i].NumberOfRelocations, f);
    fwrite("\n\tNumberOfLinenumbers: ", 1, 23, f);
    write_int(d.section[i].NumberOfLinenumbers, f);
    fwrite("\n\tCharacteristics: ", 1, 19, f);
    write_dword(d.section[i].Characteristics, f);
    fwrite("\n}\n", 1, 3, f);
}
int main() {
    const char* name = "C:\\Users\\Asus\\Downloads\\tsetup-x64.2.7.1.exe";
    FILE *f = fopen(name, "rb");
    struct DATA data;
    skip(f, 60);
    data.e_lfanew = reverse_read_4bytes(f);
    skip(f, data.e_lfanew-58);
    data.NumberOfSections = reverse_read_2bytes(f);
    skip(f, 12);
    data.SizeOfOptionalHeader = reverse_read_2bytes(f);
    skip(f, 18);
    data.AddressOfEntryPoint = reverse_read_4bytes(f);
    skip(f, data.SizeOfOptionalHeader-20);
    data.section = malloc(data.NumberOfSections*sizeof(IMAGE_SECTION_HEADER));
    for (int i = 0; i < data.NumberOfSections; ++i) {
        fread(data.section[i].Name, 1, 8, f);
        data.section[i].Misc.PhysicalAddress = reverse_read_4bytes(f);
        data.section[i].VirtualAddress = reverse_read_4bytes(f);
        data.section[i].SizeOfRawData = reverse_read_4bytes(f);
        data.section[i].PointerToRawData = reverse_read_4bytes(f);
        data.section[i].PointerToRelocations = reverse_read_4bytes(f);
        data.section[i].PointerToLinenumbers = reverse_read_4bytes(f);
        data.section[i].NumberOfRelocations = reverse_read_2bytes(f);
        data.section[i].NumberOfLinenumbers = reverse_read_2bytes(f);
        data.section[i].Characteristics = reverse_read_4bytes(f);
    }
    FILE *d = fopen("data.txt", "w");
    print_data(data, d);
    fwrite("\nSECTIONS\n", 1, 10, d);
    for (int i = 0; i < data.NumberOfSections; ++i) {
        print_section(data, i, d);
    }
    free(data.section);
    free(d);
    FILE *c = fopen("code.bin", "wb");
    int ch = fgetc(f);
    short k = 0;
    char *t = malloc(9);
    while (ch >= 0) {
        t[k] = (char)ch;
        ++k;
        if (k == 8) {
            fwrite(t, 1, 8, c);
            k = 0;
            for (int i = 0; i < 8; ++i) t[i] = '\0';
        }
        ch = fgetc(f);
    }
    if (k > 0) fwrite(t, 1, k, c);
    free(f);
    free(c);
    free(t);
    return 0;
}
