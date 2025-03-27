### osbook 環境構築
`python3-distutils`はubuntu24.04, python3-12では廃止されたので`ansible_provision.yml`の最初のほうでコメントアウトするべき．
### boot USB
USBはexFATでフォーマット．winでZドライブとして認識されていると仮定．
```sh
sudo mkdir -p /mnt/usbmem
sudo mount -t drvfs Z: /mnt/usbmem
sudo mkdir -p /mnt/usbmem/EFI/BOOT
sudo cp BOOTX64.EFI /mnt/usbmem/EFI/BOOT
sudo umount /mnt/usbmem
```
マウントしてコピーするだけ．

### qemu
```sh
qemu-img create -f raw disk.img 200M
mkfs.fat -n 'SOS' -s 2 -f 2 -R 32 -F 32 disk.img
mkdir -p mnt
sudo mount -o loop disk.img mnt
sudo mkdir -p mnt/EFI/BOOT
sudo cp BOOTX64.EFI mnt/EFI/BOOT/BOOTX64.EFI
sudo umount mnt
```
容量 200 MB のファイルにFATループデバイスとしてマウントし，`/EFI/BOOT/BOOTX64.EFI`をエントリポイントのファイルとして書き込み，ファイルシステムの認識を解除．
```sh
qemu-system-x86_64 \
 -drive if=pflash,file=$HOME/osbook/devenv/OVMF_CODE.fd \
 -drive if=pflash,file=$HOME/osbook/devenv/OVMF_VARS.fd \
 -hda disk.img
```
設定を読み込んで起動．
```sh
$HOME/osbook/devenv/run_qemu.sh BOOTX64.EFI
```
でも可．

## コンパイル
```sh
clang++ -target x86_64-elf \
-mno-red-zone -ffreestanding \
-fno-exceptions -fno-rtti -std=c++20 \
-Wall -g -O2 \
-c main.c -o main.o
ld.lld --entry KernelMain -z norelro --image-base 0x100000 --static -o kernel.elf main.o
```

