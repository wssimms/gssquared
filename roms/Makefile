# List of supported computer types
COMPUTERS = apple2_plus apple2e apple2

VENV = .venv
PYTHON = $(VENV)/bin/python3
PIP = $(VENV)/bin/pip3

$(VENV):
	python3 -m venv $(VENV)
	$(PIP) install requests

# Default target builds all computers
all: $(VENV) $(COMPUTERS)

# Generic rule for each computer type
$(COMPUTERS): %:
	@echo "Building ROMs for $@..."
	$(PYTHON) download_roms.py --machine $@

# Clean all downloaded and generated files
clean:
	rm -f */*.bin
	rm -f */*.rom
	rm -rf */base.addr
	rm -rf $(VENV)

# List available computer types
list:
	@echo "Available computer types:"
	@for computer in $(COMPUTERS); do \
		echo "  $$computer"; \
	done

.PHONY: all clean list $(COMPUTERS) 