all: sync-engine.html

sync-engine.html: $(PWD)/../engine/*.py doc-generator.py
	chmod +x doc-generator.py
	PYTHONPATH=$(PWD)/..:$(PYTHONPATH) ./doc-generator.py engine order >sync-engine.html
