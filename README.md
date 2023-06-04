# vhdtool
An about vhd and other file for tool on linux.  
Only fixed-size VHD files are currently supported, dynamic size writes are still under development...[2023.04.28]  
`vhdtool-linux` version is latest maintained code.  
`vhdtool-windows` version is no longer maintained.  

# Build:
`$ make vhdtool-[all or linux or windows]`
# Clean:
`$ make clean-[all or linux or windows]`
# Usege:
`$ vhdtool-linux -h`  
`$ vhdtool-linux -w [.Bin file] [.VHD file]`  
`$ vhdtool-linux -r [file name]`  