# Translations HOWTO

After introducing changes in [grdp file](adblock_strings.grd) it is recommended
to run `grit` tool to make sure that translation IDs in [xtb files](translations)
are matching definitions from [grdp file](adblock_strings.grd).

Command:

    ./tools/grit/grit.py -i components/adblock/android/adblock_strings.grd xmb grit_xmb_output.xml

will produce output `grit_xmb_output.xml` containing mappings between textual
IDs in [grdp file](adblock_strings.grd) (`message name` value) and numerical IDs
in [xtb files](translations) (`translation id` value).
