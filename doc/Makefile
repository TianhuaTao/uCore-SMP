#
# Build usable documents
#

ASCIIDOCTOR = asciidoctor
DITAA = ditaa
IMAGES = riscv-sbi-hsm.png ucore-memory-layout.png shared-mem.png
TARGETS = ucore-smp.pdf
TARGETS += ucore-smp.html

.PHONY: all
all: $(IMAGES) $(TARGETS)

%.png: %.ditaa
	rm -f $@
	$(DITAA) $<

ucore-memory-layout.png: ucore-memory-layout.ditaa
	rm -f $@
	$(DITAA) -E $<

shared-mem.png: shared-mem.ditaa
	rm -f $@
	$(DITAA) -E $<

%.html: %.adoc $(IMAGES)
	$(ASCIIDOCTOR) -d book -b html $<

%.pdf: %.adoc $(IMAGES) ucore-smp-theme.yml
	$(ASCIIDOCTOR) -d book -r asciidoctor-pdf -a pdf-style=ucore-smp-theme.yml -b pdf $<

.PHONY: clean
clean:
	rm -f $(TARGETS)

.PHONY: install-debs
install-debs:
	sudo apt-get install pandoc asciidoctor ditaa ruby-asciidoctor-pdf

.PHONY: install-rpms
install-rpms:
	sudo dnf install ditaa pandoc rubygem-asciidoctor rubygem-asciidoctor-pdf
