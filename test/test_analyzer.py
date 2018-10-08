#!/usr/bin/env python3

from litex.soc.tools.remote import RemoteClient

from litescope import LiteScopeAnalyzerDriver

wb = RemoteClient()
wb.open()

# # #

analyzer = LiteScopeAnalyzerDriver(wb.regs, "analyzer", debug=True)
analyzer.configure_subsampler(32)
analyzer.configure_group(0)

#analyzer.add_trigger(cond={"soc_videooverlaysoc_videooverlaysoc_videooverlaysoc_interrupt": 0x8})
#analyzer.add_rising_edge_trigger("soc_videooverlaysoc_videooverlaysoc_videooverlaysoc_interrupt")
analyzer.add_rising_edge_trigger("hdmi_out0_core_timinggenerator_source_payload_vsync")
analyzer.add_trigger(cond={"hdmi_out0_core_timinggenerator_source_payload_de":0})
analyzer.add_trigger(cond={"hdmi_out0_core_timinggenerator_source_payload_hsync":0})
#analyzer.add_trigger(cond={"soc_videooverlaysoc_hdcp_ctl_code" : 9})
#analyzer.add_trigger(cond={"soc_videooverlaysoc_analyzer_fsm2_state" : 0})

analyzer.run(offset=32, length=128)
analyzer.wait_done()
analyzer.upload()
analyzer.save("dump.vcd")

# # #

wb.close()
