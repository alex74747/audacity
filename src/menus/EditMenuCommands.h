#ifndef __AUDACITY_EDIT_MENU_COMMANDS__
#define __AUDACITY_EDIT_MENU_COMMANDS__

class AudacityProject;
class CommandManager;

class EditMenuCommands
{
   EditMenuCommands(const EditMenuCommands&);
   EditMenuCommands& operator= (const EditMenuCommands&);
public:
   EditMenuCommands(AudacityProject *project);
   void Create(CommandManager *c);
   void CreateNonMenuCommands(CommandManager *c);

   void OnUndo();
   void OnRedo();
   void OnCut();
   void OnDelete();
   void OnCopy();

   void OnPaste();
private:
   void OnPasteOver();
   bool HandlePasteText(); // Handle text paste (into active label), if any. Return true if pasted.
   bool HandlePasteNothingSelected(); // Return true if nothing selected, regardless of paste result.

   void OnDuplicate();
   void OnSplitCut();
   void OnSplitDelete();
public:
   void OnSilence();
   void OnTrim();
private:
   void OnPasteNewLabel();

   AudacityProject *const mProject;
};
#endif
