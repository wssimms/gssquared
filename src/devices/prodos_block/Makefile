MERLIN32DIR = ~/src/Merlin32_v1.1/MacOs

all: pd_block_firmware.hpp

# Download Merlin32.
#  https://www.brutaldeluxe.fr/products/crossdevtools/merlin/
# Change perms:
#  xattr -d com.apple.quarantine Merlin32

pd_block_firmware: pd_block_firmware.a65
	$(MERLIN32DIR)/Merlin32 -V . pd_block_firmware.a65

pd_block_firmware.hpp: pd_block_firmware bin2cpp.py
	python3 bin2cpp.py pd_block_firmware pd_block_firmware.hpp

clean:
	rm -f pd_block_firmware pd_block_firmware.hpp

.PHONY: all clean
