from pathlib import Path

Import("env")

def preserve_pending_usb_rx(build_env, node):
    if Path(node.srcnode().get_abspath()).resolve() != core_source:
        return node

    source = core_source.read_text(encoding="utf-8")
    before, signature, functions = source.partition("void HWCDC::begin(")
    begin, closing, after = functions.partition("\n}\n")
    clear_interrupts = "usb_serial_jtag_ll_clr_intsts_mask(USB_SERIAL_JTAG_LL_INTR_MASK);"
    if not signature or not closing or begin.count(clear_interrupts) != 1:
        raise RuntimeError("Recheck the HWCDC RX workaround for this Arduino core")

    # Let the receive handler read any packet already pending after flashing.
    preserve_rx_interrupt = (
        "usb_serial_jtag_ll_clr_intsts_mask("
        "USB_SERIAL_JTAG_LL_INTR_MASK & ~USB_SERIAL_JTAG_INTR_SERIAL_OUT_RECV_PKT);"
    )
    patched_source = before + signature + begin.replace(clear_interrupts, preserve_rx_interrupt) + closing + after
    output = Path(build_env.subst("$BUILD_DIR")) / "hwcdc_override" / core_source.name
    output.parent.mkdir(parents=True, exist_ok=True)
    if not output.exists() or output.read_text(encoding="utf-8") != patched_source:
        output.write_text(patched_source, encoding="utf-8", newline="\n")
    print("HWCDC: preserving the pending USB RX interrupt in HWCDC::begin()")
    return build_env.File(str(output))


platform = env.PioPlatform()
if "framework-arduinoespressif32" in platform.packages:
    core_package = platform.get_package_dir("framework-arduinoespressif32")
    core_source = (Path(core_package) / "cores" / "esp32" / "HWCDC.cpp").resolve()
    env.AddBuildMiddleware(preserve_pending_usb_rx, "*HWCDC.cpp")
