package main

import (
	"flag"
	"fmt"
	"os"

	"github.com/diskfs/go-diskfs"
	"github.com/diskfs/go-diskfs/disk"
	"github.com/diskfs/go-diskfs/filesystem"
	"github.com/diskfs/go-diskfs/partition/gpt"
)

const MAX_BOOTSECTOR_SIZE = 440

func main() {
	target := flag.String("target", "amd64-bios", "Target: amd64-bios")
	bootsectorPath := flag.String("bootsect", "", "Path boot sector")
	tartarusPath := flag.String("tartarus", "", "Path to tartarus.sys")
	kernelPath := flag.String("kernel", "", "Path to kernel")
	confPath := flag.String("conf", "", "Path to tartarus configuration")
	size := flag.Uint("size", 64, "Disk size in mb")
	flag.Parse()
	args := flag.Args()

	if len(args) < 1 {
		panic(fmt.Errorf("usage: mkimg [options] <image>\n"))
	}

	if *kernelPath == "" {
		panic(fmt.Errorf("Kernel is required for this target"))
	}

	if *confPath == "" {
		panic(fmt.Errorf("Tartarus configuration is required for this target"))
	}

	if err := os.RemoveAll(args[0]); err != nil {
		panic(err)
	}

	*size *= 1024 * 1024
	dsk, err := diskfs.Create(args[0], int64(*size), diskfs.Raw, diskfs.SectorSizeDefault)
	if err != nil {
		panic(err)
	}

	switch *target {
	case "amd64-bios":
		if *tartarusPath == "" {
			fmt.Println("Tartarus is required for this target")
			return
		}
		if *bootsectorPath == "" {
			fmt.Println("Bootsector is required for this target")
			return
		}

		tartarusFile, err := os.Open(*tartarusPath)
		if err != nil {
			panic(err)
		}

		bootsect, err := os.ReadFile(*bootsectorPath)
		if err != nil {
			panic(err)
		}

		tartarusInfo, err := tartarusFile.Stat()
		if err != nil {
			panic(err)
		}
		if err := dsk.Partition(&gpt.Table{
			Partitions: []*gpt.Partition{
				{Start: 2048, End: 2048 + uint64((tartarusInfo.Size()+dsk.LogicalBlocksize-1)/dsk.LogicalBlocksize), Type: "54524154-5241-5355-424F-4F5450415254", Name: "Tartarus"},
				{Start: 2048 + uint64((tartarusInfo.Size()+dsk.LogicalBlocksize-1)/dsk.LogicalBlocksize), End: uint64(dsk.Size / dsk.LogicalBlocksize), Type: "454C5953-4955-4D52-4F4F-545041525458", Name: "Elysium Root"},
			},
			ProtectiveMBR: true,
		}); err != nil {
			panic(err)
		}

		dsk.WritePartitionContents(1, tartarusFile)

		bootsectSize := int64(len(bootsect))
		if bootsectSize > MAX_BOOTSECTOR_SIZE {
			bootsectSize = MAX_BOOTSECTOR_SIZE
		}
		if _, err = dsk.File.WriteAt(bootsect[:bootsectSize], 0); err != nil {
			panic(err)
		}

		fs, err := dsk.CreateFilesystem(disk.FilesystemSpec{Partition: 2, FSType: filesystem.TypeFat32, VolumeLabel: "Elysium Root"})
		if err != nil {
			panic(err)
		}

		copyFile := func(srcPath string, destPath string) {
			fileData, err := os.ReadFile(srcPath)
			if err != nil {
				panic(err)
			}
			file, err := fs.OpenFile(destPath, os.O_CREATE|os.O_RDWR)
			if err != nil {
				panic(err)
			}
			if _, err := file.Write(fileData); err != nil {
				panic(err)
			}
		}

		copyFile(*kernelPath, "/kernel.elf")
		copyFile(*confPath, "/tartarus.cfg")
	}
}
