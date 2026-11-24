const minimumMatterFlashBytes = 8 * 1024 * 1024;
const unknownFlashSizeMessage = 'Could not verify the flash size required by the 8 MB Matter firmware.';
const guardedDialogs = new WeakSet();

class MatterFlashSizeError extends Error {}

export function guardMatterFlashSize(loader) {
    const dialog = Array.from(document.querySelectorAll('ewt-install-dialog'))
        .find(candidate => candidate.port === loader.transport.device);
    if (!dialog?.manifestPath ||
        !new URL(dialog.manifestPath, document.baseURI).pathname.endsWith('/manifest_full_8mb.json')) {
        return;
    }

    if (!guardedDialogs.has(dialog)) {
        // ESP Web Tools 10.5.2 replaces initialization errors with generic text.
        const willUpdate = dialog.willUpdate.bind(dialog);
        dialog.willUpdate = changedProperties => {
            const state = dialog._installState;
            const error = state?.details?.details;
            if (state?.state === 'error' && error instanceof MatterFlashSizeError) {
                state.message = error.message;
            }
            willUpdate(changedProperties);
        };
        guardedDialogs.add(dialog);
    }

    // ESP Web Tools calls flashId before downloading parts, erasing or writing.
    loader.flashId = async () => {
        let flashSize;
        try {
            flashSize = await loader.detectFlashSize();
        } catch (cause) {
            throw new MatterFlashSizeError(unknownFlashSizeMessage, {cause});
        }
        const flashBytes = flashSize ? loader.flashSizeBytes(flashSize) : 0;
        if (!(flashBytes > 0)) {
            throw new MatterFlashSizeError(unknownFlashSizeMessage);
        }
        if (flashBytes < minimumMatterFlashBytes) {
            const detectedSize = flashSize.replace(/(KB|MB)$/, ' $1');
            throw new MatterFlashSizeError(
                `This board has ${detectedSize} of flash. The Matter firmware requires at least 8 MB.`
            );
        }
    };
}
