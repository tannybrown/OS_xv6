# Goal
##### Implement the pread, pwrite system call.
##### pread and pwrite system call additionally gets the offset parameter to indicate the data offset of the file.

# Design
##### Refer to the pred,pwrite function of Linux
##### In the existing fileread and filewrite, offset is additionally received as a argument, and offset is handed over to the readi and writei functions so that the file can be read and written at any desired location.

# Implementation
## pread
![image](https://user-images.githubusercontent.com/121401159/230775172-4a97c6c1-1f26-4cd5-a6ca-87506ed0bb34.png)
##### The pread is almost same to the form of the fileread function.
##### However, conventional readi receives f->off as a argument, whereas it receives offset as a argument and gives readi an off as a argument.
##### This allows you to specify the location of the file and read it.
## pwrite
![image](https://user-images.githubusercontent.com/121401159/230775180-9ee34aab-ff1b-4322-85f6-f705853aa3dd.png)
##### The pwrite is almost same to the form of the filewrite function.
##### However, conventional writei receives f->off as a argument, whereas it receives offset as a argument and gives writei an off as a argument.
##### This allows you to specify the location of the file and write it.

## writei

##### An error occurred in pwrite during the test and i trace and found that return -1 was found in the writei condition statement.
##### It was found that if off is larger than the current file size, it becomes return-1.
##### Because multiple threads access a single file, I realized that an attempt to access a space beyond the file size occurred.
##### So I erased this part.
![image](https://user-images.githubusercontent.com/121401159/230775195-8a334d22-9249-4f70-852f-27eb05278e66.png)
# Test
##### In the test, 10 threads access a 16MB file and write and read it like milstone 1.
![image](https://user-images.githubusercontent.com/121401159/230775208-a5237358-9c2d-45ee-bbfc-166c99b2565f.png)
![image](https://user-images.githubusercontent.com/121401159/230775229-771943d2-76de-41b1-a31f-8075758ca83d.png)
##### Some of the printf prints were cut because of the context switch. But each thread can be checked to read and write to a predetermined location with its own off.
