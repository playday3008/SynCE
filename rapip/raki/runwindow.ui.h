/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/


void RunWindow::openFileDialog(KURLRequester *requester)
{
    KURL url("rapip:/");
    KFileDialog *fd = requester->fileDialog();
    fd->setFilter("*.exe|Executable (*.exe)");
    fd->setURL(url);
//    requester->setURL("rapip:/");
    fd->setMode(KFile::File | KFile::ExistingOnly);
}


void RunWindow::executeProgram()
{
    WCHAR* wide_program = NULL;
    WCHAR* wide_parameters = NULL;
    PROCESS_INFORMATION info = {0, 0, 0, 0 };

    KURL url(runUrlRequester->url());
    
    if (Ce::rapiInit()) {
        if ((wide_program = Ce::wpath_from_upath(url.path()))) {    
            if (!Ce::createProcess(wide_program, wide_parameters,
                    NULL,
                    NULL,
                    false,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    &info)) {
            }
            close();
        }
    }

    Ce::destroy_wpath(wide_program);
    Ce::destroy_wpath(wide_parameters);
    
    Ce::rapiUninit();
}
