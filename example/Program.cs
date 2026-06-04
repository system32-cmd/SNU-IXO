using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Linq;
using System.Text;

namespace KERNEL
{
    class UEFI
    {
        public struct BootInfo
        {
            public byte[] instruction;
            public bool safemode;
            public int isBoot;
        }
        public static void Boot(string instructions, string OSName)
        {
            Console.Clear();
            Console.WriteLine();
            BootInfo BootInfo = new BootInfo();
            BootInfo.instruction = Encoding.ASCII.GetBytes(instructions);
            BootInfo.safemode = false;
            BootInfo.isBoot = 1;
            char[] validBoot = instructions.Substring(510, 2).ToCharArray();
            if (validBoot[0] == 0x55 && validBoot[1] == 0xAA && BootInfo.isBoot != 0)
            {;
                Console.WriteLine("VALID BOOT SECTOR");
                Console.WriteLine("LOADING EFI");
                Thread.Sleep(1000);
                Console.WriteLine($"Booting {OSName}");
                Thread.Sleep(1000);
                Console.Clear();
                //
            }
            else
            {
                Console.WriteLine("Invalid boot sector. Halting.");
                while (true) { }
            }
        }
    }
}