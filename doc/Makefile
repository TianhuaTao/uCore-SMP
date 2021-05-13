#
# Build usable documents
#

ASCIIDOCTOR = asciidoctor
DITAA = ditaa
IMAGES = riscv-sbi-hsm.png
TARGETS = ucore-smp.pdf
TARGETS += ucore-smp.html

.PHONY: all
all: $(IMAGES) $(TARGETS)

%.png: %.ditaa
	rm -f $@
	$(DITAA) $<

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
