/*
  ==============================================================================

    Prefab.cpp
    Created: 25 Sep 2016 10:14:16am
    Author:  Ryan Challinor

  ==============================================================================
*/

#include "Prefab.h"
#include "ModularSynth.h"
#include "PatchCableSource.h"

Prefab::Prefab()
{
   mModuleContainer.SetOwner(this);
   mPrefabName = "";
}

Prefab::~Prefab()
{
}

void Prefab::CreateUIControls()
{
   IDrawableModule::CreateUIControls();
   
   mSaveButton = new ClickButton(this, "save", 95, 2);
   mLoadButton = new ClickButton(this, "load", mSaveButton, kAnchor_Right);
   mDisbandButton = new ClickButton(this, "disband", mLoadButton, kAnchor_Right);
   
   mModuleCable = new PatchCableSource(this, kConnectionType_Special);
   mModuleCable->SetManualPosition(10, 10);
   AddPatchCableSource(mModuleCable);
}

string Prefab::GetTitleLabel()
{
   if (mPrefabName != "")
      return "prefab: "+mPrefabName;
   return "prefab";
}

void Prefab::Poll()
{
   float xMin,yMin;
   GetPosition(xMin, yMin);
   for (auto* module : mModuleContainer.GetModules())
   {
      xMin = MIN(xMin, module->GetPosition().x);
      yMin = MIN(yMin, module->GetPosition().y - 30);
   }
   
   int xOffset = GetPosition().x - xMin;
   int yOffset = GetPosition().y - yMin;
   for (auto* module : mModuleContainer.GetModules())
      module->SetPosition(module->GetPosition(true).x + xOffset, module->GetPosition(true).y + yOffset);
   
   if (abs(GetPosition().x - xMin) >= 1 || abs(GetPosition().y - yMin) >= 1)
      SetPosition(xMin, yMin);
}

void Prefab::OnClicked(int x, int y, bool right)
{
   IDrawableModule::OnClicked(x, y, right);
   
   if (y > 0 && !right)
      TheSynth->SetGroupSelectContext(&mModuleContainer);
}

namespace
{
   const float paddingX = 10;
   const float paddingY = 10;
}

void Prefab::DrawModule()
{
   if (Minimized() || IsVisible() == false)
      return;

   mSaveButton->Draw();
   mLoadButton->Draw();
   mDisbandButton->Draw();
   DrawTextNormal("add/remove", 18, 14);

   if (CanAddGroup())
      DrawTextNormal("type + to add group", 3, 34);
   
   mModuleContainer.Draw();
}

bool Prefab::CanAddGroup()
{
   return !TheSynth->GetGroupSelectedModules().empty() && VectorContains(static_cast<IDrawableModule*>(this), TheSynth->GetGroupSelectedModules());
}

void Prefab::PostRepatch(PatchCableSource* cableSource, bool fromUserClick)
{
   IDrawableModule* module = dynamic_cast<IDrawableModule*>(cableSource->GetTarget());
   if (module)
   {
      if (!VectorContains(module, mModuleContainer.GetModules()))
         mModuleContainer.TakeModule(module);
      else
         GetOwningContainer()->TakeModule(module);
   }
   cableSource->Clear();
}

void Prefab::GetModuleDimensions(float& width, float& height)
{
   float x,y;
   GetPosition(x, y);
   width = 215;
   height = 40;
   
   //if (PatchCable::sActivePatchCable && PatchCable::sActivePatchCable->GetOwningModule() == this)
   //   return;
      
   for (auto* module : mModuleContainer.GetModules())
   {
      ofRectangle rect = module->GetRect();
      if (rect.x - x + rect.width + paddingX > width)
         width = rect.x - x + rect.width + paddingX;
      if (rect.y - y + rect.height + paddingY > height)
         height = rect.y - y + rect.height + paddingY;
   }
}

void Prefab::ButtonClicked(ClickButton* button)
{
   if (button == mSaveButton)
   {
      FileChooser chooser("Save prefab as...", File(ofToDataPath("prefabs/prefab.pfb")), "*.pfb", true, false, TheSynth->GetMainComponent()->getTopLevelComponent());
      if (chooser.browseForFileToSave(true))
      {
         string savePath = chooser.getResult().getRelativePathFrom(File(ofToDataPath(""))).toStdString();
         SavePrefab(savePath);
      }
   }
   
   if (button == mLoadButton)
   {
      FileChooser chooser("Load prefab...", File(ofToDataPath("prefabs")), "*.pfb", true, false, TheSynth->GetMainComponent()->getTopLevelComponent());
      if (chooser.browseForFileToOpen())
      {
         string loadPath = chooser.getResult().getRelativePathFrom(File(ofToDataPath(""))).toStdString();
         LoadPrefab(ofToDataPath(loadPath));
      }
   }
   
   if (button == mDisbandButton)
   {
      auto modules = mModuleContainer.GetModules();
      for (auto* module : modules)
         GetOwningContainer()->TakeModule(module);
      GetOwningContainer()->DeleteModule(this);
   }
}

void Prefab::KeyPressed(int key, bool isRepeat)
{
   if (key == '=')
   {
      if (CanAddGroup())
      {
         for (auto* module : TheSynth->GetGroupSelectedModules())
         {
            if (module != this && !VectorContains(module, mModuleContainer.GetModules()))
               mModuleContainer.TakeModule(module);
         }
      }
   }
}

void Prefab::SavePrefab(string savePath)
{
   ofxJSONElement root;
   
   root["modules"] = mModuleContainer.WriteModules();
   
   stringstream ss(root.getRawString(true));
   string line;
   string lines;
   while (getline(ss,line,'\n'))
   {
      const char* pos = strstr(line.c_str(), " : \"$");
      if (pos != nullptr)
      {
         bool endsWithComma = line[line.length()-1] == ',';
         ofStringReplace(line, pos, " : \"\"");
         if (endsWithComma)
            line += ",";
      }
      lines += line + '\n';
   }
   
   UpdatePrefabName(savePath);
   
   FileStreamOut out(ofToDataPath(savePath).c_str());
   
   out << lines;
   mModuleContainer.SaveState(out);
}

void Prefab::LoadPrefab(string loadPath)
{
   ScopedMutex mutex(TheSynth->GetAudioMutex(), "LoadPrefab()");
   ScopedLock renderLock(*TheSynth->GetRenderLock());
   
   mModuleContainer.Clear();
   
   FileStreamIn in(ofToDataPath(loadPath).c_str());

   assert(in.OpenedOk());
   
   string jsonString;
   in >> jsonString;
   ofxJSONElement root;
   bool loaded = root.parse(jsonString);
   
   if (!loaded)
   {
      TheSynth->LogEvent("Couldn't load, error parsing "+loadPath, kLogEventType_Error);
      TheSynth->LogEvent("Try loading it up in a json validator", kLogEventType_Error);
      return;
   }
   
   UpdatePrefabName(loadPath);
   
   mModuleContainer.LoadModules(root["modules"]);
   
   mModuleContainer.LoadState(in);
}

void Prefab::UpdatePrefabName(string path)
{
   vector<string> tokens = ofSplitString(path, GetPathSeparator());
   mPrefabName = tokens[tokens.size() - 1];
   ofStringReplace(mPrefabName, ".pfb", "");
}

void Prefab::SaveLayout(ofxJSONElement& moduleInfo)
{
   IDrawableModule::SaveLayout(moduleInfo);
   moduleInfo["modules"] = mModuleContainer.WriteModules();
}

void Prefab::LoadLayout(const ofxJSONElement& moduleInfo)
{
   mModuleContainer.LoadModules(moduleInfo["modules"]);
   SetUpFromSaveData();
}

void Prefab::SetUpFromSaveData()
{
}

namespace
{
   const int kSaveStateRev = 0;
}

void Prefab::SaveState(FileStreamOut& out)
{
   IDrawableModule::SaveState(out);

   out << kSaveStateRev;
   out << mPrefabName;
}

void Prefab::LoadState(FileStreamIn& in)
{
   IDrawableModule::LoadState(in);

   if (!ModuleContainer::DoesModuleHaveMoreSaveData(in))
      return;  //this was saved before we added versioning, bail out

   int rev;
   in >> rev;
   LoadStateValidate(rev <= kSaveStateRev);

   in >> mPrefabName;
}
