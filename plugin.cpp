#include "UI.h"
#include<cmath>
#include<algorithm>
#include<string_view>
#include <spdlog/sinks/basic_file_sink.h>

namespace logger = SKSE::log;

void SetupLog() {
    auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
    auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
    auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
    auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
    auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
    spdlog::set_default_logger(std::move(loggerPtr));
    spdlog::set_level(spdlog::level::err);
    spdlog::flush_on(spdlog::level::err);
}


void CallPapyrusFunction(const char* FunctionName, RE::BSScript::IFunctionArguments* args) {
    auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
    if (!vm) {
        logger::error("VM not found");
        return;
    }

    const RE::BSFixedString scriptName("EM03QuestScript");
    const RE::BSFixedString functionName(FunctionName);

    auto* quest = UI::SCScriptQuest;
    if (!quest) {
        logger::error("Quest not found");
        return;
    }

    auto* policy = vm->GetObjectHandlePolicy();
    if (!policy) {
        logger::error("HandlePolicy not found");
        return;
    }

    RE::VMHandle questHandle = policy->GetHandleForObject(quest->GetFormType(), quest);
    auto callback = RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>();

    // Call the function
    bool success = vm->DispatchMethodCall(questHandle, scriptName, functionName, args, callback);
    // VM handle to the object (quest), name of script attached to the object, name of Papyrus function, arguments

    if (!success) {
        logger::error("Failed to dispatch method call to Papyrus.");
    }
}


//function OpenSpellCraftMenu() global native
void OpenSpellCraftMenu(RE::StaticFunctionTag*, RE::TESQuest* theQuest,
                        const std::vector<RE::TESForm*> formArray) {
    logger::info("OpenSpellCraftMenu Called"); 

    if (theQuest) {
        UI::SCScriptQuest = theQuest;
    } else {
        logger::error("couldn't get the Quest");
    }

    RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
    auto* dataHandler = RE::TESDataHandler::GetSingleton();
    const auto& allSpells = dataHandler->GetFormArray<RE::SpellItem>();
    for (auto* spell : allSpells) {
        if (spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell && player->HasSpell(spell)) {
            auto Skill = spell->GetAssociatedSkill();
            switch (Skill) {
                case RE::ActorValue::kAlteration:
                    UI::ChoseEffectMenu::AltSpells.push_back(spell);
                    break;
                case RE::ActorValue::kConjuration:
                    UI::ChoseEffectMenu::ConSpells.push_back(spell);
                    break;
                case RE::ActorValue::kDestruction:
                    UI::ChoseEffectMenu::DesSpells.push_back(spell);
                    break;
                case RE::ActorValue::kIllusion:
                    UI::ChoseEffectMenu::IllSpells.push_back(spell);
                    break;
                case RE::ActorValue::kRestoration:
                    UI::ChoseEffectMenu::ResSpells.push_back(spell);
                    break;
                default:
                    UI::ChoseEffectMenu::ElseSpells.push_back(spell);
            }
        }
    }

    UI::ChoseEffectMenu::LearnedEffects.clear();
     for (auto* form : formArray) {
        if (form && form->GetFormType() == RE::FormType::MagicEffect) {
            auto* effect = form->As<RE::EffectSetting>();
            if (effect) {
                UI::ChoseEffectMenu::LearnedEffects.push_back(effect);
            }
        }
    }

    UI::SpellCraftMenu::OverideEffect = false;
    UI::SpellCraftMenu::OverideBase = nullptr;
    UI::SpellCraftMenu::OverSchoolInt = 5;
    UI::SpellCraftMenu::SpellCraftWindow->IsOpen = true;
}

//Spell function OpenChoseSpellMenu() global native
void OpenChoseSpellMenu(RE::StaticFunctionTag*, RE::TESQuest* theQuest) {
    if (theQuest) {
        UI::SCScriptQuest = theQuest;
    } else {
        logger::error("couldn't get the Quest");
    }

    logger::info("OpenChoseSpellMenu Called");
    RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
    for (auto spell : player->GetActorRuntimeData().addedSpells) {
        if (spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
            auto Skill = spell->GetAssociatedSkill();
            switch (Skill) {
                case RE::ActorValue::kAlteration:
                    UI::ChoseEffectMenu::AltSpells.push_back(spell);
                    break;
                case RE::ActorValue::kConjuration:
                    UI::ChoseEffectMenu::ConSpells.push_back(spell);
                    break;
                case RE::ActorValue::kDestruction:
                    UI::ChoseEffectMenu::DesSpells.push_back(spell);
                    break;
                case RE::ActorValue::kIllusion:
                    UI::ChoseEffectMenu::IllSpells.push_back(spell);
                    break;
                case RE::ActorValue::kRestoration:
                    UI::ChoseEffectMenu::ResSpells.push_back(spell);
                    break;
                default:
                    UI::ChoseEffectMenu::ElseSpells.push_back(spell);
            }
        }
    }
    UI::ChoseEffectMenu::Erasing = true;
    UI::ChoseEffectMenu::ChoseEffectWindow->IsOpen = true;
}   

//function SetEffectStuff(MagicEffect theEffect) global native
void SetEffectStuff(RE::StaticFunctionTag*, RE::EffectSetting* theEffect) {
    logger::info("SetEffectStuff Called");
    if (!theEffect) {
        logger::error("Invalid EffectSetting pointer(s)");
        return;
    }

    theEffect->data.delivery = UI::SpellCraftMenu::OverideBase->GetDelivery();
    theEffect->data.castingType = UI::SpellCraftMenu::OverideBase->GetCastingType();
}

//function GiveSettings(int TomeCost, bool MustHavePerk) global native
void GiveSettings(RE::StaticFunctionTag*, int32_t TomeCost, int32_t EffectCost1, bool MustHavePerk, bool FullScreen1, bool ShowogCost1,
                   float ME, float DM, float AM, int32_t SliderMax1) { 
    UI::MCP::tomeCost = TomeCost; 
    UI::MCP::EffectGCost = EffectCost1;
    UI::ShowPerkEffects = MustHavePerk;
    UI::MCP::FullScreen = FullScreen1;
    //UI::BCostMult = BCM;
    UI::MagExp = ME;
    UI::DMult = DM;
    UI::AMult = AM;
    UI::SliderMax = SliderMax1;
    UI::MCP::ShowOgCost = ShowogCost1;
}

bool PapyrusFunctions(RE::BSScript::IVirtualMachine * vm) { 
    vm->RegisterFunction("OpenSpellCraftMenu", "EM03SKSEFunctions", OpenSpellCraftMenu);
    vm->RegisterFunction("OpenChoseSpellMenu", "EM03SKSEFunctions", OpenChoseSpellMenu);
    vm->RegisterFunction("SetEffectStuff", "EM03SKSEFunctions", SetEffectStuff);
    vm->RegisterFunction("GiveSettings", "EM03SKSEFunctions", GiveSettings);
    return true; }


const char* GetCastingtypeS(RE::MagicSystem::CastingType EffectCT) { 
    switch (EffectCT) {
        case RE::MagicSystem::CastingType::kConstantEffect:
            return "Constant Effect";
            break;
        case RE::MagicSystem::CastingType::kFireAndForget:
            return "Instant";
            break;
        case RE::MagicSystem::CastingType::kConcentration:
            return "Concentration";
            break;
        default:
            logger::error("Casting Type not one of the used for spells? None?");
            return "Error: don't recognise this";
    }
}

const char* GetDeliverytypeS(RE::MagicSystem::Delivery EffectD) {
    switch (EffectD) {
        case RE::MagicSystem::Delivery::kAimed:
            return "Aimed";
            break;
        case RE::MagicSystem::Delivery::kSelf:
            return "Self";
            break;
        case RE::MagicSystem::Delivery::kTargetActor:
            return "Target Actor";
            break;
        case RE::MagicSystem::Delivery::kTargetLocation:
            return "Target Location";
            break;
        case RE::MagicSystem::Delivery::kTouch:
            return "Touch";
            break;
        default:
            logger::error("Delivery not one of the 5 in commonlib? None?");
            return "Error: don't recognise this";
    }
}

const char* GetSchoolS(RE::ActorValue Skill) {
    switch (Skill) {
        case RE::ActorValue::kAlteration:
            return "Escuela: Alteracion";
            break;
        case RE::ActorValue::kConjuration:
            return "Escuela: Conjuracion";
            break;
        case RE::ActorValue::kDestruction:
            return "Escuela: Destruccion";
            break;
        case RE::ActorValue::kIllusion:
            return "Escuela: Ilusion";
            break;
        case RE::ActorValue::kRestoration:
            return "Escuela: Restauracion";
            break;
        default:
            return "Sin Escuela";
    }
}



void StopSpellCrafting(bool cancelled, bool noName) {
    using namespace UI;
    using namespace SpellCraftMenu;

     if (!cancelled) {
        noSchool = false;
        auto maxIt = std::max_element(CostList.begin(), CostList.end());
        auto HighestCostid = static_cast<int>(std::distance(CostList.begin(), maxIt));
        if (SpellEffects.empty() || (OverideEffect && (OverideBase == nullptr || OverSchoolInt == 5))) {
            Empty = true;
            return;
        } else if (!OverideEffect && SpellEffects[HighestCostid]->GetMagickSkill() == RE::ActorValue::kNone) {
            noSchool = true;
            Empty = true;
            return;
        } else if (SpellName[0] == '\0' && !noName) {
            Empty = true;
            return;
        } else {
            RE::BSFixedString SpellNameStr(SpellName);
            if (!OverideEffect) {
                OverideBase = SourceSpells[HighestCostid];
            }
            auto args = RE::MakeFunctionArguments(
                (RE::BSFixedString)SpellNameStr, (std::vector<RE::EffectSetting*>)SpellEffects,
                (std::vector<int32_t>)MagList, (std::vector<int32_t>)AreaList, (std::vector<int32_t>)DurationList,
                (int32_t)OverSchoolInt, (std::vector<float>)CostList, (RE::SpellItem*)OverideBase,
                (int32_t)HighestCostid, (bool)OverideEffect);
            CallPapyrusFunction("CraftTheSpell", args);
        }
    }
    SpellCraftWindow->IsOpen = false;
    SpellName[0] = '\0';
    SpellEffects.clear();
    CostList.clear();
    MagList.clear();
    AreaList.clear();
    DurationList.clear();
    SourceSpells.clear();
    DList.clear();
    ChoseEffectMenu::AltSpells.clear();
    ChoseEffectMenu::ConSpells.clear();
    ChoseEffectMenu::DesSpells.clear();
    ChoseEffectMenu::IllSpells.clear();
    ChoseEffectMenu::ResSpells.clear();
    ChoseEffectMenu::ElseSpells.clear();
    ChoseEffectMenu::LearnedEffects.clear();
    Empty = false;
    noSchool = false;
}

float AddExtraAD(float ListNum, uint32_t OgNum, int min) {
    float NumExtra = 0;
    if (OgNum < min) {
        if (ListNum < min) {
            NumExtra = (ListNum - OgNum) / 20.0f;
            if (NumExtra < 0) {
                NumExtra = 0;
            }
        } else {
            NumExtra = (min / 20.0f) - (OgNum / 20.0f);
        }
    }
    return NumExtra;
}

void __stdcall UI::SpellCraftMenu::RenderWindow() {
    auto viewport = ImGui::GetMainViewport();
    if (MCP::FullScreen) {
        ImGui::SetNextWindowPos(viewport->Pos, ImGuiCond_Appearing, ImVec2{0, 0});
        ImGui::SetNextWindowSize(ImVec2{viewport->Size}, ImGuiCond_Appearing);  
    } else {
        auto center = ImGui::ImVec2Manager::Create();
        ImGui::ImGuiViewportManager::GetCenter(center, viewport);
        ImGui::SetNextWindowPos(*center, ImGuiCond_Appearing, ImVec2{0.5f, 0.6f});
        ImGui::ImVec2Manager::Destroy(center);
        ImGui::SetNextWindowSize(ImVec2{viewport->Size.x * 0.6f, viewport->Size.y * 0.6f}, ImGuiCond_Appearing);
    }
    ImGui::Begin("Craft Spell##EM03SpellCrafting", nullptr, ImGuiWindowFlags_None);  
    
    if (ImGui::Button("Cancelar")) {
        StopSpellCrafting(true, false);
        CallPapyrusFunction("CloseBook", RE::MakeFunctionArguments());
    }
    ImGui::SameLine();
    if (ImGui::Button("Finalizar")) {StopSpellCrafting(false, false);}

    if (Empty) {
        bool Empty1 = SpellEffects.empty();
        bool Empty2 = (OverideEffect && OverideBase == nullptr);
        if (Empty1 && Empty2) {
            ImGui::Text("El conjuro no tiene efectos y el efecto personalizado no tiene un efecto base del cual copiar los visuales.");
        } else if (Empty1) {
            ImGui::Text("El conjuro no tiene efectos.");
        } else if (Empty2) {
            ImGui::Text("El efecto personalizado no tiene un efecto base del cual copiar los visuales.");
        } else if (OverideEffect && OverSchoolInt == 5) {
            ImGui::Text("No has elegido una escuela para el efecto personalizado.");
        } else if (noSchool && !OverideEffect) {
            ImGui::Text("Si el efecto con el mayor costo de magicka no tiene escuela, debes reemplazarlo con un efecto personalizado.");
        } else if (SpellName[0] == '\0') {
            ImGui::Text("El conjuro no tiene nombre, estas seguro de que deseas finalizar?");
            if (ImGui::Button("Si")) {
                StopSpellCrafting(false, true);
            }
            ImGui::SameLine();
            if (ImGui::Button("No")) {
                Empty = false;
            }
        }
    }

    ImGui::BeginChild("Scrolling");
    ImGui::InputText("Nombre del Conjuro", SpellName, 128);
  
    ImGui::Checkbox("Reemplazar entrega, visuales y escuela con un efecto personalizado? (De lo contrario sera tomado del de mayor costo)", &OverideEffect);
    if (OverideEffect) {
        const char* Lable = "Elegir un Conjuro para usar como base";
        if (OverideBase) {
            auto* Overidemgf = OverideBase->GetCostliestEffectItem()->baseEffect;
            ImGui::Text("Usar visuales, entrega y tipo de lanzamiento de este efecto: %s (%s, %s)",
                        Overidemgf->GetFullName(), GetDeliverytypeS(Overidemgf->data.delivery),
                        GetCastingtypeS(Overidemgf->data.castingType));
            ImGui::SameLine();
            Lable = "Cambiar el Efecto";
        }
        if (ImGui::Button(Lable)) {
            ChangeE1 = true;
            SpellCraftWindow->IsOpen = false;
            ChoseEffectMenu::ChoseEffectWindow->IsOpen = true;
        }
        ImGui::Combo("Escuela", &OverSchoolInt, Schools, 6);
    }

    ImGui::Text("");
    for (std::size_t i = 0; i< SpellEffects.size(); i++) {
        RE::EffectSetting* Effect = SpellEffects[i];
      
        ImGui::Text(Effect->GetFullName());
        ImGui::SameLine();
        ImGui::PushID(i);
        if (ImGui::Button("Remover")) {
            SpellEffects.erase(SpellEffects.begin() + i);
            CostList.erase(CostList.begin() + i);
            MagList.erase(MagList.begin() + i);
            AreaList.erase(AreaList.begin() + i);
            DurationList.erase(DurationList.begin() + i);
            SourceSpells.erase(SourceSpells.begin() + i);
            DList.erase(DList.begin() + i);
            break;
        }
        ImGui::PopID();
        ImGui::SameLine;
        ImGui::Text("(Entrega: %s, Tipo de lanzamiento: %s, %s)", GetDeliverytypeS(Effect->data.delivery),
                    GetCastingtypeS(Effect->data.castingType), GetSchoolS(Effect->GetMagickSkill()));

        auto Flags = Effect->data.flags;
         if (!(Flags & RE::EffectSetting::EffectSettingData::Flag::kNoMagnitude)) {
            ImGui::PushID(&MagList[i]);
            ImGui::SliderInt("Magnitude", &MagList[i], 0, SliderMax);
            ImGui::PopID();
        }

        if (!(Flags & RE::EffectSetting::EffectSettingData::Flag::kNoArea)) {
            ImGui::PushID(&AreaList[i]);
            ImGui::SliderInt("Area", &AreaList[i], 0, SliderMax);
            ImGui::PopID();
        }
        if (!(Flags & RE::EffectSetting::EffectSettingData::Flag::kNoDuration)) {
            ImGui::PushID(&DurationList[i]);
            ImGui::SliderInt("Duration", &DurationList[i], 0, SliderMax);
            ImGui::PopID();
        }   
        
        float theCost = -1;
        uint32_t OgDur = 0;
        uint32_t OgArea = 0;
        if (SourceSpells[i]) {
            for (auto effect : SourceSpells[i]->effects) {
                if (effect->baseEffect == Effect) {
                    theCost = effect->cost;
                    OgDur = effect->GetDuration();
                    OgArea = effect->GetArea();
                    break;
                }
            }
        }
        auto AreaDiv = AMult;
        if (OgArea > AMult) {AreaDiv = OgArea;}
        float AreaExtra = AddExtraAD(AreaList[i], OgArea, AreaDiv);
        float durExtra = AddExtraAD(DurationList[i], OgDur, 10);

        float Dcost = std::max(DurationList[i] / 10.0f, 1.0f) + durExtra;
        CostList[i] = DList[i] * DMult * powf((std::max(MagList[i], 1) * Dcost), 1.1) *
                      (std::max(AreaList[i] / AreaDiv, 1.0f) + AreaExtra);
        ImGui::Text("Costo de magia: %.0f", CostList[i]);
        if (MCP::EffectGCost != 0) {
            ImGui::SameLine();
            ImGui::Text("Costo en oro: %.0f", CostList[i] * MCP::EffectGCost);
        }
        if (MCP::ShowOgCost && theCost != -1) {
            ImGui::SameLine();
            ImGui::Text("Costo en el hechizo original: %.0f", theCost);
        }
        ImGui::Text("");
    }

    if (ImGui::Button("Añadir efectos")) {
        logger::info("Add an Effect");
        SpellCraftWindow->IsOpen = false;
        ChoseEffectMenu::ChoseEffectWindow->IsOpen = true;
    }
    ImGui::EndChild();
    ImGui::End();
}



void GenSpellSchoolMenu(std::vector<RE::SpellItem*> School, ImVec4 Color, const char* Title, std::string_view SearchText) {  // I couldn't think of a name
    ImGui::TextColored(Color, Title);
    for (auto Spell : School) {
        std::string SpellName = Spell->GetFullName();
        if (UI::ChoseEffectMenu::Searching && !SpellName.starts_with(SearchText)) {
            continue;
        }
        
        std::string label = SpellName + "##" + std::string(std::to_string(Spell->formID));
        if (ImGui::Button(label.c_str())) {
            if (UI::ChangeE1) {
                UI::SpellCraftMenu::OverideBase = Spell;
                UI::ChangeE1 = false;
                UI::ChoseEffectMenu::ChoseEffectWindow->IsOpen = false;
                UI::SpellCraftMenu::SpellCraftWindow->IsOpen = true;

            } else if (UI::ChoseEffectMenu::Erasing) {
                UI::ChoseEffectMenu::SpellToErase = Spell;
            }else {
                auto Delivery = Spell->GetDelivery();
                auto Player = RE::PlayerCharacter::GetSingleton();
                bool check = UI::ShowPerkEffects && Delivery == RE::MagicSystem::Delivery::kSelf;
                for (auto& effectItem : Spell->effects) {
                    auto& Conditions = effectItem->conditions;
                    if (check && !Conditions.IsTrue(Player, Player)) {
                        logger::info("las condiciones no se cumplieron para que el efecto se mostrara");
                        continue;
                    }
                    auto Duration = effectItem->GetDuration();
                    auto Cost = effectItem->cost;
                    auto Mag = effectItem->GetMagnitude();

                    RE::EffectSetting* mgef = effectItem->baseEffect;
                    UI::SpellCraftMenu::SpellEffects.push_back(mgef);
                    UI::SpellCraftMenu::MagList.push_back(Mag);
                    UI::SpellCraftMenu::AreaList.push_back(effectItem->GetArea());
                    UI::SpellCraftMenu::DurationList.push_back(Duration);
                    UI::SpellCraftMenu::CostList.push_back(Cost);
                    UI::SpellCraftMenu::SourceSpells.push_back(Spell); 

                    float BaseCost1 = mgef->data.baseCost;
                    float TrueBaseCost = 0;
                    if (BaseCost1 > 0.01) {
                        float calcCost = powf((std::max(Mag, 1.0f) * std::max(Duration / 10.0f, 1.0f)), 1.1);
                        TrueBaseCost = (Cost / calcCost) * UI::BCostMult;
                    }
                    UI::SpellCraftMenu::DList.push_back(TrueBaseCost);
                }
                UI::BCostMult = 1;
                UI::ChoseEffectMenu::ChoseEffectWindow->IsOpen = false;
                UI::SpellCraftMenu::SpellCraftWindow->IsOpen = true;
            }
        }
        ImGui::SameLine();
        ImGui::Text("%s, %s", GetDeliverytypeS(Spell->GetDelivery()), GetCastingtypeS(Spell->GetCastingType()));
    }
}

void __stdcall UI::ChoseEffectMenu::RenderWindow() {
    auto viewport = ImGui::GetMainViewport();
    if (MCP::FullScreen) {
        ImGui::SetNextWindowPos(viewport->Pos, ImGuiCond_Appearing, ImVec2{0, 0});
        ImGui::SetNextWindowSize(ImVec2{viewport->Size}, ImGuiCond_Appearing);
    } else {
        auto center = ImGui::ImVec2Manager::Create();
        ImGui::ImGuiViewportManager::GetCenter(center, viewport);
        ImGui::SetNextWindowPos(*center, ImGuiCond_Appearing, ImVec2{0.5f, 0.6f});
        ImGui::ImVec2Manager::Destroy(center);
        ImGui::SetNextWindowSize(ImVec2{viewport->Size.x * 0.6f, viewport->Size.y * 0.6f},ImGuiCond_Appearing);
    }
    ImGui::Begin("Hechizos y Efectos Conocidos##EM03SpellCrafting", nullptr, ImGuiWindowFlags_None);

    if (ImGui::Button("Cancelar")) {
        ChangeE1 = false;
        ChoseEffectWindow->IsOpen = false;
        if (Erasing) {
            Erasing = false;
            SpellToErase = nullptr;
            StopSpellCrafting(true, false);
            CallPapyrusFunction("CloseBook", RE::MakeFunctionArguments());
        } else {
            SpellCraftMenu::SpellCraftWindow->IsOpen = true;
        }
    }
    if (SpellToErase) {
        ImGui::Text("¿Estás seguro de que deseas borrar y olvidar %s?", SpellToErase->GetFullName());
        if (ImGui::Button("Si")) {
            UI::ChoseEffectMenu::ChoseEffectWindow->IsOpen = false;
            CallPapyrusFunction("EraseSpell", RE::MakeFunctionArguments((RE::SpellItem*)SpellToErase));
            UI::ChoseEffectMenu::Erasing = false;
            SpellToErase = nullptr;
            StopSpellCrafting(true, false);
        }
        ImGui::SameLine();
        if (ImGui::Button("No")) {
            SpellToErase = nullptr;
        }
    } else {
        ImGui::SameLine();
        ImGui::InputText("Buscar", SearchName, 128);
        std::string_view searchText = SearchName;
        if (SearchName[0] != '\0') {
            Searching = true;
        } else {
            Searching = false;
        }
        ImGui::BeginChild("Scrolling");

        if (!AltSpells.empty()) {GenSpellSchoolMenu(AltSpells, ImVec4(0, 0, 1, 1), "Hechizos de Alteración", searchText);}
        if (!ConSpells.empty()) {GenSpellSchoolMenu(ConSpells, ImVec4(0.5, 0, 0.5, 1), "Hechizos de Conjuración", searchText);}
        if (!DesSpells.empty()) {GenSpellSchoolMenu(DesSpells, ImVec4(1, 0, 0, 1), "Hechizos de Destrucción", searchText);}
        if (!IllSpells.empty()) {GenSpellSchoolMenu(IllSpells, ImVec4(0, 1, 1, 1), "Hechizos de Ilusión", searchText);}
        if (!ResSpells.empty()) {GenSpellSchoolMenu(ResSpells, ImVec4(1, 1, 0, 1), "Hechizos de Restauración", searchText);}
        if (!ElseSpells.empty()) {GenSpellSchoolMenu(ElseSpells, ImVec4(1, 0, 1, 1), "Otros Hechizos (Sin Escuela)", searchText);}

        if (!LearnedEffects.empty() && !ChangeE1) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Efectos de Alquimia");  // RGBA
            for (auto Effect : LearnedEffects) {
                std::string EffectName = Effect->GetFullName();
                if (UI::ChoseEffectMenu::Searching && !EffectName.starts_with(searchText)) {
                    continue;
                }
                std::string label = EffectName + "##" + std::string(std::to_string(Effect->formID));
                if (ImGui::Button(label.c_str())) {
                    SpellCraftMenu::SpellEffects.push_back(Effect);
                    SpellCraftMenu::CostList.push_back(0);
                    SpellCraftMenu::MagList.push_back(15);
                    SpellCraftMenu::AreaList.push_back(0);
                    SpellCraftMenu::DurationList.push_back(10);
                    SpellCraftMenu::SourceSpells.push_back(nullptr);
                    SpellCraftMenu::DList.push_back(BCostMult * Effect->data.baseCost);
                    BCostMult = 1;
                    ChoseEffectWindow->IsOpen = false;
                    SpellCraftMenu::SpellCraftWindow->IsOpen = true;
                }
                ImGui::SameLine();
                ImGui::Text("%s, %s", GetDeliverytypeS(Effect->data.delivery),
                            GetCastingtypeS(Effect->data.castingType));
            }
        }
       ImGui::End(); 
    }
    ImGui::End();
}

void __stdcall UI::MCP::Render() {
    if (!SCScriptQuest) {
        bool workednow = false;
        RE::TESForm* form = RE::TESForm::LookupByEditorID("EM03SCScriptQuest");
        if (form) {
            if (auto* quest = form->As<RE::TESQuest>()) {
                UI::SCScriptQuest = quest;
                workednow = true;
                logger::error("Didn't have Quest already but sucecessfully found it now.");
            } else {
                logger::error("Form 'EM03SCScriptQuest' is not a TESQuest.");
            }
        } else {
            logger::error("Failed to find form with EditorID 'EM03SCScriptQuest'.");
        }
        if (!workednow) {
            ImGui::Text("Error: No se pudieron obtener los scripts de Papyrus necesarios para guardar o cargar la configuración.");
            logger::error("SCScriptQuest is not filled/ is nullptr");
        }
    } else if (!GotSettings) {
        CallPapyrusFunction("GetSettings", RE::MakeFunctionArguments());
        GotSettings = true;
    }
    

    if (ImGui::Button("Guardar Configuración")) {
        auto args = RE::MakeFunctionArguments((int32_t)tomeCost, (int32_t)EffectGCost, (bool)ShowPerkEffects,
                    (bool)FullScreen, (bool)ShowOgCost, (float)MagExp, (float)DMult, (float)AMult, (int32_t)SliderMax);
        CallPapyrusFunction("ApplySettings", args);
        GotSettings = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Restaurar a Predeterminados")) {
        ShowPerkEffects = true;
        FullScreen = false;
        ShowOgCost = false;
        BCostMult = 1.0;
        MagExp = 1.1;
        DMult = 1;
        AMult = 15;
        SliderMax = 100;
        tomeCost = 0;
        EffectGCost = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Agregar un Tomo de Creación de Hechizos al Inventario")) {
        CallPapyrusFunction("AddSCTome" , RE::MakeFunctionArguments());
    }
    ImGui::Text("Algunos cambios se aplicarán de inmediato, pero otros no se guardarán hasta que presiones Guardar Configuración");

    ImGui::BeginChild("Scrolling");
    
    ImGui::Text("");
    ImGui::Text("Establecer un costo en oro para adquirir el Tomo de Creación de Hechizos:");
    ImGui::InputInt("Costo en Oro", &tomeCost, 10, 100);
    ImGui::Text("");

    ImGui::Checkbox("Debes tener la ventaja para agregar su efecto si proviene de un hechizo propio", &ShowPerkEffects);
    ImGui::Checkbox("Los menús de Creación de Hechizos son a pantalla completa (no cambies esto mientras un menú esté abierto)", &FullScreen);
    ImGui::Text("");

    ImGui::Text("Valor máximo que pueden alcanzar los deslizadores de Magnitud, Área y Duración:");
    ImGui::InputInt("Ingresa un Número", &SliderMax, 10, 100);
    ImGui::Text("");

    ImGui::Checkbox("Mostrar el costo del efecto en su hechizo original", &ShowOgCost);
    ImGui::Text("Roughly, Magicka Cost =  B x BaseCost x (Magnitude x (Duration/10))^M x (Area/A)");
    ImGui::SliderFloat("B", &DMult, 0, 5);
    ImGui::SliderFloat("M", &MagExp, 0, 5);
    ImGui::SliderFloat("A", &AMult, 0, 25);
    ImGui::Text("Multiplicar el costo solo de los próximos efectos añadidos:");
    ImGui::SliderFloat("Multiplicador de Costo del Próximo Efecto", &BCostMult, 0, 5);
    ImGui::Text("Cambiar cualquiera de estos valores después de crear un hechizo no modificará su costo retroactivamente.");
    ImGui::Text("");

    ImGui::Text("Establecer un costo en oro por el costo de magia de cada efecto:");
    ImGui::InputInt("Oro/Magia", &EffectGCost, 1, 10);
    ImGui::EndChild();
}



void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kPostLoad) {
        SKSE::GetPapyrusInterface()->Register(PapyrusFunctions);
        UI::SpellCraftMenu::SpellCraftWindow = SKSEMenuFramework::AddWindow(UI::SpellCraftMenu::RenderWindow);
        UI::ChoseEffectMenu::ChoseEffectWindow = SKSEMenuFramework::AddWindow(UI::ChoseEffectMenu::RenderWindow);
        SKSEMenuFramework::SetSection("Fourth Era Spell Crafting");
        SKSEMenuFramework::AddSectionItem("Main Page", UI::MCP::Render);
    }

    else if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        RE::TESForm* form = RE::TESForm::LookupByEditorID("EM03SCScriptQuest");
        if (form) {
            if (auto* quest = form->As<RE::TESQuest>()) {
                UI::SCScriptQuest = quest;
            } else {
                logger::error("Form 'EM03SCScriptQuest' is not a TESQuest.");
            }
        } else {
            logger::error("Failed to find form with EditorID 'EM03SCScriptQuest'.");
        }
    }
     else if (message->type == SKSE::MessagingInterface::kPostLoadGame) {
        UI::MCP::GotSettings = false;
        RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
        constexpr std::string_view kNameId = "EM03ID_"sv;
        for (auto Spell : player->GetActorRuntimeData().addedSpells) {
             if (Spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                 auto mgef = Spell->effects[0]->baseEffect;
                 std::string_view Name = mgef->GetFullName();
                 if (Name == kNameId) {
                     mgef->data.delivery = Spell->GetDelivery();
                     mgef->data.castingType = Spell->GetCastingType();

                 }
             }
        }
    }
}


SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SKSE::Init(skse);
    SetupLog();
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}
