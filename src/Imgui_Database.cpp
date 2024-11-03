#include "Imgui_Database.h"
#include "imgui_internal.h"
#define IMGUI_DATABASE_MAX_NUM_COLUMN 10
#define IMGUI_DATABASE_MIN_NUM_COLUMN 0

void ImGuiDatabase::ImGuiDatabaseWindow::Clear()
{
	loadDatabase = false;
	loadTable = false;
	select = 0;
}

Mugenshuen::string_t ImGuiDatabase::ImGuiDatabaseWindow::Name()
{
	return "database_window";
}

void ImGuiDatabase::ImGuiDatabaseWindow::Draw(ImGuiDatabaseAppication* app)
{
	
	int count = 0;
	bool focusedTable = false;
	Mugenshuen::string_t focusedTableName = "";
	auto numDB = app->GetNumDatabsaes();
	for (int i = 0; i < numDB; ++i)
	{
		auto names = app->GetNamesDatabase(i);
		ImGuiTreeNodeFlags flags = !app->IsLoadedDatabase(names) ? ImGuiTreeNodeFlags_Leaf : 0;
		flags |= select == count ? ImGuiTreeNodeFlags_Selected : 0;
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
		if (ImGui::TreeNodeEx(names.C_Str(), flags))
		{
			if (ImGui::IsItemFocused() || ImGui::IsItemClicked(ImGuiMouseButton_Left | ImGuiMouseButton_Right))
			{
				select = count;
				selectDatabase = i;
			}

			if (ImGui::IsWindowFocused())
			{
				if (!app->IsLoadedDatabase(names) && 
					(ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) || select == count && ImGui::IsKeyPressed(ImGuiKey_Enter, false)))
				{
					app->CloseTable(true);
					app->CloseDatabase(true);
					app->LoadDatabase(names);
				}

			}

			for (int j = 0; app->IsLoadedDatabase(names)&&j < app->GetNumTables(); ++j)
			{
				count++;
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf;
				flags |= select == count ? ImGuiTreeNodeFlags_Selected : 0;
				if (ImGui::TreeNodeEx(app->GetNamesTable(j).C_Str(), flags))
				{

					if (ImGui::IsWindowFocused() &&
						(ImGui::IsItemClicked(ImGuiMouseButton_Left) &&
							ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) ||
							select == count &&
							ImGui::IsKeyPressed(ImGuiKey_Enter, false)))
					{
						app->SaveTable();
						app->OpenTableWindow();
						app->LoadTable(app->GetNamesTable(j));
					}

					if (ImGui::IsItemFocused())
					{
						select = count;
						selectDatabase = i;
					}
					if (!focusedTable)
					{
						focusedTable = select == count;
						focusedTableName = app->GetNamesTable(j).C_Str();
					}
					ImGui::TreePop();
				}
			}

			ImGui::TreePop();
		}

		if (ImGui::IsItemFocused() || ImGui::IsItemClicked(ImGuiMouseButton_Left | ImGuiMouseButton_Right))
		{
			select = count;
			selectDatabase = i;
		}
		count++;
	}
	if (numDB == 0)
	{
		select = -1;
		selectDatabase = -1;
	}
	if (ImGui::BeginPopup("command_database"))
	{
		if (ImGui::MenuItem("Create Database") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
		{
			app->OpenCreateDatabaseWindow();
			ImGui::CloseCurrentPopup();
		}
		else if
			(selectDatabase != -1 && !app->IsLoadedDatabase(app->GetNamesDatabase(selectDatabase)) &&
				(ImGui::MenuItem("Load Database") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))))
		{
			app->CloseTable(true);
			app->CloseDatabase(true);
			app->LoadDatabase(app->GetNamesDatabase(selectDatabase));
			ImGui::CloseCurrentPopup();
		}
		else if 
			(selectDatabase != -1 &&
			(ImGui::MenuItem("Close Database") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))))
		{
			app->CloseTable(true);
			app->CloseDatabase(true);
			app->CloseRecordWindow();
			app->CloseTableWindow();
			app->CloseRelationalWindow();
			ImGui::CloseCurrentPopup();
		}
		else if
			(selectDatabase != -1 &&
			(ImGui::MenuItem("Delete Database") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))))
		{
			app->DeleteDatabase(app->GetNamesDatabase(selectDatabase));
			ImGui::CloseCurrentPopup();
			select = -1;
			if (numDB - 1 == selectDatabase)
				selectDatabase--;
		}
		else if ((app->IsLoadedDatabase(app->GetNamesDatabase(selectDatabase))) &&
			(ImGui::MenuItem("Create Table") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))))
		{
			app->OpenCreateTableWindow();
			ImGui::CloseCurrentPopup();

		}
		else if (focusedTable &&
			(ImGui::MenuItem("Delete Table") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))))
		{
			app->DeleteTable(focusedTableName);
			ImGui::CloseCurrentPopup();

		}
		ImGui::EndPopup();
	}

}

void ImGuiDatabase::ImGuiDatabaseWindow::DrawFocuse(ImGuiDatabaseAppication* app)
{

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsKeyPressed(ImGuiKey_Menu))
	{
		ImGui::OpenPopup("command_database");
	}
}


void ImGuiDatabase::ImGuiRecordWindow::Create(ImGuiDatabaseAppication* app, int numColumn, const MugenScript::MugenScriptDatabaseValue* names, const MugenScript::DATABASE_VALUE_TYPE* type, const MugenScript::MugenScriptDatabaseValue* initials, MugenScript::MugenScriptDatabaseValue* dst)
{
	if (open)
		Close();
	state = RECORD_WINDOW_STATE_NONE;
	resultState = RECORD_WINDOW_STATE_CREATE;
	open = true;
	first = true;
	this->numColumn = numColumn;
	this->names = names;
	this->types = type;
	record = dst;
	input = new ImGuiValueInput[numColumn]();
	buttonName = "Create";
	for (int i = 0; i < numColumn; ++i)
	{
		record[i] = initials[i];
		if (types[i] == MugenScript::DATABASE_VALUE_TYPE_STR || types[i] == MugenScript::DATABASE_VALUE_TYPE_EX_SCRIPT)
			input[i].strCapacity = strlen(record[i].value.string) + 1;
	}
}

void ImGuiDatabase::ImGuiRecordWindow::Edit(ImGuiDatabaseAppication* app, int id,int numColumn, const MugenScript::MugenScriptDatabaseValue* names,  const MugenScript::DATABASE_VALUE_TYPE* type, MugenScript::MugenScriptDatabaseValue* dst)
{
	if (open)
		Close();
	state = RECORD_WINDOW_STATE_NONE;
	resultState = RECORD_WINDOW_STATE_REPLACE;
	open = true;
	first = true;
	this->id = id;
	this->numColumn = numColumn;
	this->names = names;
	this->types = type;
	record = dst;
	buttonName = "Edit";
	input = new ImGuiValueInput[numColumn]();
	for (int i = 0; i < numColumn; ++i)
	{
		if (type[i] == MugenScript::DATABASE_VALUE_TYPE_EX_REL)
		{
			record[i] = app->GetRecord(id)[i];
			if (record[i].type == MugenScript::DATABASE_VALUE_TYPE_STR)
				input[i].strCapacity = strlen(record[i].value.string);
		}

		if (types[i] == MugenScript::DATABASE_VALUE_TYPE_STR || types[i] == MugenScript::DATABASE_VALUE_TYPE_EX_SCRIPT)
			input[i].strCapacity = strlen(record[i].value.string) + 1;
	}
}

void ImGuiDatabase::ImGuiRecordWindow::Close()
{
	open = false;
	numColumn = 0;
	names = nullptr;
	types = nullptr;
	record = nullptr;
	buttonName = "";
	resultState = RECORD_WINDOW_STATE_NONE;
	id = -1;
	delete[] input;
}

int ImGuiDatabase::ImGuiRecordWindow::EditID()
{
	return id;
}

bool ImGuiDatabase::ImGuiRecordWindow::IsOpen() const
{
	return open;
}

ImGuiDatabase::RECORD_WINDOW_STATE ImGuiDatabase::ImGuiRecordWindow::State() const
{
	return state;
}

void ImGuiDatabase::ImGuiRecordWindow::Draw(ImGuiDatabaseAppication* app)
{
	app->OnlyInput(Name());
	if (!ImGui::IsWindowFocused())
		ImGui::SetWindowFocus();
	for (int i = 0; i < numColumn; ++i)
		input[i].Input(app,id, types[i], record[i], names[i].value.string);
	

	if (ImGui::Button(buttonName.C_Str()) || ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))
	{
		if (resultState == RECORD_WINDOW_STATE_CREATE)
			app->CreateRecordData(app->GetNumColumns(), record);
		else if (resultState == RECORD_WINDOW_STATE_REPLACE)
			app->EditRecordData(id, record);
		app->CloseRecordWindow();
		Close();
		app->ClearOnlyInput();
	}
	if (ImGui::Button("Cancel")||ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))
	{
		app->CloseRecordWindow();
		Close();
		app->ClearOnlyInput();
	}
	first = false;
}

Mugenshuen::string_t ImGuiDatabase::ImGuiRecordWindow::Name()
{
	return "record_window";
}

ImGuiWindowFlags ImGuiDatabase::ImGuiRecordWindow::Flags()
{
	return 0;
}

Mugenshuen::string_t ImGuiDatabase::ImGuiDatabaseTableView::Name()
{
	return "table_window";
}

void ImGuiDatabase::ImGuiDatabaseTableView::Draw(ImGuiDatabaseAppication* app)
{
	if (app->GetNumColumns() == 0)
		return;
	bool tblWindowFocus = ImGui::IsWindowFocused();
	bool isRecordFocus = false;
	bool isOpenPopupColumnRelation = false;
	bool isOpenPopupColumnCondition = false;
	auto& io = ImGui::GetIO();
	imguiTable.Set(app->GetNumColumns(), app->GetNamesColumn());
	imguiTable.Begin(Name().C_Str(), ImGuiTableBgTarget_CellBg | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY);
	
	if (imguiTable.BeginColumn())
	{
		for (int i = 0; i < app->GetNumColumns(); ++i)
		{
			ImGui::TableNextColumn();

			if (ImGui::Selectable(app->GetNamesColumn()[i].value.string) ||
				(ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
			{
				selectColumn = i;
				if (app->GetTypesColumn()[i] == MugenScript::DATABASE_VALUE_TYPE_EX_REL|| app->GetTypesColumn()[i] == MugenScript::DATABASE_VALUE_TYPE_EX_TRIGGER)
					isOpenPopupColumnRelation = true;
			}

		}
		imguiTable.EndColumn();
	}

	if (tblWindowFocus && ImGui::IsKeyPressed(ImGuiKey_Enter, false))
		ImGui::SetWindowFocus();
	for (int i = 0; i < app->GetNumRecords(); ++i)
	{
		int id = app->GetRecordID(i);

		if (imguiTable.BeginRecord(app->GetRecord(id)))
			select = i;
		if (ImGui::IsItemClicked())
			select = i;
		if (
			!tblWindowFocus&&
			ImGui::IsItemFocused() &&
			select == i &&
			(
				ImGui::IsItemClicked() &&
				ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) ||
				ImGui::IsKeyPressed(ImGuiKey_Enter, false))
			)
		{
			app->OpenRecordWindow();
			app->EditRecord(id);
		}
		if (ImGui::IsItemFocused())
			isRecordFocus = true;

		imguiTable.EndRecord();
	}

	imguiTable.End();

	if ((tblWindowFocus || ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) && (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsKeyPressed(ImGuiKey_Menu)))
	{
		ImGui::OpenPopup("command");
		recordForcus = isRecordFocus;
	}

	if (ImGui::BeginPopup("command"))
	{
		if (ImGui::MenuItem("Create") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
		{
			app->OpenRecordWindow();
			app->CreateRecord();
			ImGui::CloseCurrentPopup();
		}
		if (recordForcus&&(ImGui::MenuItem("Edit") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))))
		{
			app->OpenRecordWindow();
			app->EditRecord(app->GetRecordID(select));
			ImGui::CloseCurrentPopup();
		}
		if (recordForcus&&(ImGui::MenuItem("Delete") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))))
		{
			int id = app->GetRecordID(select);
			app->DeleteRecord(id);
			if (app->GetNumRecords() <= select)
				select = app->GetNumRecords() - 1;
			ImGui::CloseCurrentPopup();
		}
		if (ImGui::MenuItem("Save") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
		{
			app->SaveTable();
			ImGui::CloseCurrentPopup();
		}
		if (ImGui::MenuItem("Close") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
		{
			app->FocuseDatabaseWindow();
			app->CloseTableWindow();
			app->CloseTable(true);
			app->CloseRelationalWindow();
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if(isOpenPopupColumnRelation)
		ImGui::OpenPopup("relation");

	if (ImGui::BeginPopup("relation"))
	{
		if (ImGui::MenuItem("create") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
		{
			app->OpenCreateRelationalWindow(app->GetNamesColumn()[selectColumn].value.string);
			app->CreateRelational();
			ImGui::CloseCurrentPopup();
		}
		if (ImGui::MenuItem("edit") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
		{
			app->OpenCreateRelationalWindow(app->GetNamesColumn()[selectColumn].value.string);
			app->EditRelational();
			ImGui::CloseCurrentPopup();
		}
		if (ImGui::MenuItem("open") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
		{
			app->OpenRelationalWindow(selectColumn);
			ImGui::CloseCurrentPopup();
		}
		if (ImGui::MenuItem("close") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
		{
			app->CloseRelationalWindow();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	} 

	if (isOpenPopupColumnCondition)
		ImGui::OpenPopup("condition");

	if (ImGui::BeginPopup("condition"))
	{
		if (ImGui::MenuItem("set") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
		{
			app->OpenCreateConditionWindow(selectColumn);
			ImGui::CloseCurrentPopup();
		}
		if (ImGui::MenuItem("open") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
		{
			app->OpenConditionWindow(selectColumn);
			ImGui::CloseCurrentPopup();
		}
		if (ImGui::MenuItem("close") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
		{
			app->CloseConditionWindow();
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void ImGuiDatabase::ImGuiDatabaseTableView::DrawFocuse(ImGuiDatabaseAppication* app)
{
	//else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && 0 < select)
	//	select--;
	//else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && select < app->GetNumRecords() - 1)
	//	select++;
}

int ImGuiDatabase::ImGuiValueInput::Input(ImGuiDatabaseAppication* app, int record, MugenScript::DATABASE_VALUE_TYPE type, MugenScript::MugenScriptDatabaseValue& value, Mugenshuen::string_t name)
{
	struct CapacityUserData
	{
		char** str;
		int* capacity;
	};

	auto resize = [](ImGuiInputTextCallbackData* data)
		{

			auto userdata = (CapacityUserData*)data->UserData;
			if (data->BufTextLen < *userdata->capacity)
				return 0;
			auto str = userdata->str;
			auto pre = *str;
			*str = new char[data->BufSize + 20];
			if (pre)
			{
				strcpy_s(*str, data->BufSize + 20, pre);
				delete[] pre;
			}
			data->BufSize += 20;
			*userdata->capacity += 20;
			return 0;
		};

	int ret = 0;
	switch (type)
	{
	case MugenScript::DATABASE_VALUE_TYPE_INT:
		ret=ImGui::InputInt(name.C_Str(), &value.value.ivalue);
		break;
	case MugenScript::DATABASE_VALUE_TYPE_DBL:
		ret=ImGui::InputDouble(name.C_Str(), &value.value.dvalue);
	
	break;
	case MugenScript::DATABASE_VALUE_TYPE_STR:
	{
		CapacityUserData capa = { &value.value.string,&strCapacity };
		ret=ImGui::InputText(name.C_Str(), value.value.string, strCapacity, ImGuiInputTextFlags_CallbackResize, resize, &capa);
		break;
	}
	case MugenScript::DATABASE_VALUE_TYPE_EX_REL:
	{
		auto type = app->GetTypeRelation(name);
		if (type == MugenScript::DATABASE_VALUE_TYPE_EX_REL)
			Input(app,record, MugenScript::DATABASE_VALUE_TYPE_INVALID, value, name);
		else
			Input(app,record, type, value, name);

		break;
	}
	case MugenScript::DATABASE_VALUE_TYPE_EX_TRIGGER:
	{
		auto type = app->GetTypeRelation(name);
		if (type == MugenScript::DATABASE_VALUE_TYPE_EX_REL)
			Input(app,record, MugenScript::DATABASE_VALUE_TYPE_INVALID, value, name);
		else
			Input(app,record, type, value, name);

		int commandFlagMaskCond =
			MugenScript::MugenScriptTriggerBehavior_CondRelationTrue |
			MugenScript::MugenScriptTriggerBehavior_CondRelationFalse |
			MugenScript::MugenScriptTriggerBehavior_CondEdge |
			MugenScript::MugenScriptTriggerBehavior_CondEdgeUp |
			MugenScript::MugenScriptTriggerBehavior_CondEdgeDown;

		int commandFlagMaskAct =
			MugenScript::MugenScriptTriggerBehavior_ActOne |
			MugenScript::MugenScriptTriggerBehavior_AlwaysTrue |
			MugenScript::MugenScriptTriggerBehavior_AlwaysFalse;

		ImGui::Text("Condition");
		ImGui::CheckboxFlags("true", &value.userData, MugenScript::MugenScriptTriggerBehavior_CondRelationTrue);
		ImGui::SameLine();
		ImGui::CheckboxFlags("false", &value.userData, MugenScript::MugenScriptTriggerBehavior_CondRelationFalse);
		ImGui::SameLine();
		ImGui::CheckboxFlags("edge", &value.userData, MugenScript::MugenScriptTriggerBehavior_CondEdge);
		ImGui::SameLine();
		ImGui::CheckboxFlags("edge up", &value.userData, MugenScript::MugenScriptTriggerBehavior_CondEdgeUp);
		ImGui::SameLine();
		ImGui::CheckboxFlags("edge down", &value.userData, MugenScript::MugenScriptTriggerBehavior_CondEdgeDown);

		if (((commandFlagMaskCond & value.userData) != (commandFlagMaskCond & triggerSelected)) && value.userData != 0)
			value.userData ^= triggerSelected & commandFlagMaskCond;

		ImGui::Text("Action");
		ImGui::CheckboxFlags("one", &value.userData, MugenScript::MugenScriptTriggerBehavior_ActOne);
		ImGui::SameLine();
		ImGui::CheckboxFlags("always true", &value.userData, MugenScript::MugenScriptTriggerBehavior_AlwaysTrue);
		ImGui::SameLine();
		ImGui::CheckboxFlags("always false", &value.userData, MugenScript::MugenScriptTriggerBehavior_AlwaysFalse);

		if (((commandFlagMaskAct & value.userData) != (commandFlagMaskAct & triggerSelected)) && value.userData != 0)
			value.userData ^= triggerSelected & commandFlagMaskAct;


		triggerSelected = value.userData;
		break;

	}

	case MugenScript::DATABASE_VALUE_TYPE_EX_EVENT:
	{
		CapacityUserData capa = { &value.value.string,&strCapacity };
		ret = ImGui::InputText(name.C_Str(), value.value.string, strCapacity, ImGuiInputTextFlags_CallbackResize, resize, &capa);
		ImGui::Text("relation trigger");
		for (int i = 0; i < app->GetNumColumns(); ++i)
		{
			char label[2] = { '1' + i,'\0' };
			if (name != app->GetNamesColumn()[i].value.string)
				ImGui::CheckboxFlags(label, &value.userData, 1 << i);
			ImGui::SameLine();
		}
		ImGui::NewLine();
		break;
	}
	case MugenScript::DATABASE_VALUE_TYPE_EX_SCRIPT:
	{
		char viewStr[10] = {};
		for (int i = 0; i < 7 && i < value.size(); ++i)
			viewStr[i] = value.value.string[i];
		for (int i = 8; i < 10; ++i)
			viewStr[i] = '.';
		if (value.size() != 1)
			ImGui::Text(" %s ", viewStr, 10);
		else
			ImGui::Text("{ empty script }");
		ImGui::SameLine();
		if (ImGui::Button("execute"))
		{
			app->OpenDebugWindow("DxLib/MugenScript_DxLib.exe",value.value.string);
		}
		ImGui::SameLine();
		if (ImGui::Button("edit"))
		{
			app->EditScript(app->CurrentDatabase() + "_script", value.value.string, &value);
		}
		break;
	}

	default:
	{
		char viewStr[] = "NaN";
		ImGui::InputText(name.C_Str(), viewStr, sizeof(viewStr));
	}
		break;
	}
	return ret;
}


void ImGuiDatabase::ImGuiDatabaseTable::Set(int column, const MugenScript::MugenScriptDatabaseValue* name, Callback idCallback)
{
	numColumn = column;
	names.reserve(numColumn);
	for (int i = 0; i < numColumn; ++i)
		names[i] = name[i].value.string;
	callback = idCallback;
}

void ImGuiDatabase::ImGuiDatabaseTable::Begin(const char* label, ImGuiTableFlags flags)
{
	
	result = ImGui::BeginTable(label, numColumn + 1, flags);
	if (result)
	{
		ImGui::TableNextColumn();
		ImGui::Text("id");
	}
}

bool ImGuiDatabase::ImGuiDatabaseTable::BeginColumn()
{
	if (result)
	{
		record = 0;
	}
	return result;
}

bool ImGuiDatabase::ImGuiDatabaseTable::BeginRecord(MugenScript::MugenScriptDatabaseValue* values)
{
	Mugenshuen::string_t idStr;
	int id = record;
	int pos = 0;
	idStr = id;
	this->values = values;
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::PushID(id);

	return ImGui::Selectable(idStr.C_Str(),false, ImGuiSelectableFlags_SelectOnNav);
}

void ImGuiDatabase::ImGuiDatabaseTable::EndColumn()
{
	ImGui::TableNextRow();
}

void ImGuiDatabase::ImGuiDatabaseTable::EndRecord()
{
	//if(tree)
	//	ImGui::TreePop();

	ImGui::PopID();
	for (int column = 0; column < numColumn; ++column)
	{
		Mugenshuen::string_t viewStr = GetStringValue(values[column]);
		viewStr = GetStringOneLine(viewStr);
		ImGui::TableSetColumnIndex(column + 1);
		ImGui::PushID(record * 3 + column);
		ImGui::Text(viewStr.C_Str());
		ImGui::PopID();
	}
	record++;
}
void ImGuiDatabase::ImGuiDatabaseTable::End()
{
	if (result)
		ImGui::EndTable();
}

Mugenshuen::string_t ImGuiDatabase::ImGuiDatabaseTable::GetStringValue(const MugenScript::MugenScriptDatabaseValue& v)
{
	MugenScript::DATABASE_VALUE_TYPE type = v.type;
	Mugenshuen::string_t ret;
	switch (type)
	{
	case MugenScript::DATABASE_VALUE_TYPE_INT:
		ret = v.value.ivalue;
		break;
	case MugenScript::DATABASE_VALUE_TYPE_DBL:
		ret = v.value.dvalue;
		break;
	case MugenScript::DATABASE_VALUE_TYPE_STR:
		ret = v.value.string;
		break;
	case MugenScript::DATABASE_VALUE_TYPE_NUM_TYPE:
		break;
	default:
		break;
	}
	return ret;
}

Mugenshuen::string_t ImGuiDatabase::ImGuiDatabaseTable::GetStringOneLine(Mugenshuen::string_t str)
{
	Mugenshuen::string_t ret = str;
	ret.reserve(str.Capacity());
	int count = 0;
	while (count < str.Lengh() && (str[count] != '\n' && str[count] != '\0'))
	{
		ret[count] += str[count];
		count++;
	}
	if (count < str.Lengh() && str[count] == '\n')
	{
		ret += "...";
	}
	return ret;
}

void ImGuiDatabase::ImGuiDatabaseDirectoryWindow::Update(MugenScript::MugenScriptDatabaseEditor* editor, int(callback)(MugenScript::MugenScriptDatabaseEditor*, Mugenshuen::string_t&))
{
	if (ImGui::Begin("directory_window"))
	{
		it.entry();
		while (!it.last())
		{
			Mugenshuen::string_t name = it.get().filename().stem().string().c_str();
			if (ImGui::TreeNodeEx(name.C_Str(), ImGuiTreeNodeFlags_Leaf))
			{
				if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					callback(editor, name);
				ImGui::TreePop();
			}
			it.next();
		}
	}
	ImGui::End();
}

ImGuiDatabase::ImGuiDatabaseDirectoryWindow::ImGuiDatabaseDirectoryWindow():
	it(FileSystem::fm.file("resource"))
{
}

void ImGuiDatabase::ImGuiDatabaseWindowBase::Update(ImGuiDatabaseAppication* app, ImGuiWindowFlags flags)
{
	if (ImGui::Begin(Name().C_Str(), 0, Flags() | flags))
		Draw(app);
	if (ImGui::IsWindowFocused())
		DrawFocuse(app);
	ImGui::End();
}

void ImGuiDatabase::ImGuiDatabaseCommandLoadTable::Do()
{

}

void ImGuiDatabase::ImGuiDatabaseAppication::Update()
{
	dbWindow.Update(this,GetFlag(dbWindow.Name()));
	if (activeTableWindow)
		tblWindow.Update(this, GetFlag(tblWindow.Name()));
	if (activeRecordWindow)
		recWindow.Update(this, GetFlag(recWindow.Name()));
	if (activeCreateDatabaseWindow)
		cdbWindow.Update(this, GetFlag(cdbWindow.Name()));
	if (activeCreateTableWindow)
		ctblWindow.Update(this, GetFlag(ctblWindow.Name()));
	if (activeCreateRelationWindow)
		crelWindow.Update(this, GetFlag(crelWindow.Name()));
	if (activeRelationWindow)
		relWindow.Update(this, GetFlag(relWindow.Name()));
	if (activeCreateConditionWindow)
		ccndWindow.Update(this, GetFlag(ccndWindow.Name()));
	if (activeConditionWindow)
		cndWindow.Update(this, GetFlag(cndWindow.Name()));
	if (activeDebugWindow)
		dbgWindow.Update(this, GetFlag(dbgWindow.Name()));
	if (activeScriptManager)
		scriptManager.Update();
	if (activeScriptManager && !activeRecordWindow)
	{
		scriptManager.EndEditScript();
		activeScriptManager = false;
	}
	ExecuteEvent();

}

void ImGuiDatabase::ImGuiDatabaseAppication::OpenDatabaseWindow()
{
	eventQueue.push(ImGuiDatabaseEvent_OpenDatabaseWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::OpenTableWindow()
{
	eventQueue.push(ImGuiDatabaseEvent_OpenTableWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::OpenRecordWindow()
{
	eventQueue.push(ImGuiDatabaseEvent_OpenRecordWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::OpenCreateDatabaseWindow()
{
	eventQueue.push(ImGuiDatabaseEvent_OpenCreateDatabaseWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::OpenCreateTableWindow()
{
	eventQueue.push(ImGuiDatabaseEvent_OpenCreateTableWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::OpenRelationalWindow(int columnID)
{
	Mugenshuen::string_t relDB;
	Mugenshuen::string_t relTable;
	Mugenshuen::string_t relColumn;
	GetRelationalPath(columnID, relDB, relTable, relColumn);
	relWindow.SetColumnID(columnID);
	relWindow.SetRelationalPath(relDB, relTable, relColumn);
	eventQueue.push(ImGuiDatabaseEvent_OpenRelationalWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::OpenConditionWindow(int columnID)
{
	cndWindow.SetColumnID(columnID);
	eventQueue.push(ImGuiDatabaseEvent_OpenConditionWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::OpenDebugWindow(Mugenshuen::string_t processPath, Mugenshuen::string_t script)
{
	dbgWindow.PushProcessRequest(processPath);
	dbgWindow.PushLoadScript(script);
	eventQueue.push(ImGuiDatabaseEvent_OpenDebugWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::OpenCreateRelationalWindow(Mugenshuen::string_t columnName)
{
	crelWindow.SetColumnName(columnName);
	eventQueue.push(ImGuiDatabaseEvent_OpenCreateRelationalWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::OpenCreateConditionWindow(int columnID)
{
	eventQueue.push(ImGuiDatabaseEvent_OpenCreateConditionWindow);
	ccndWindow.SetColumnID(columnID);
}

void ImGuiDatabase::ImGuiDatabaseAppication::CloseDatabaseWindow()
{
	eventQueue.push(ImGuiDatabaseEvent_CloseDatabaseWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::CloseTableWindow()
{
	eventQueue.push(ImGuiDatabaseEvent_CloseTableWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::CloseRecordWindow()
{
	eventQueue.push(ImGuiDatabaseEvent_CloseRecordWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::CloseCreateDatabaseWindow()
{
	eventQueue.push(ImGuiDatabaseEvent_CloseCreateDatabaseWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::CloseCreateTableWindow()
{
	eventQueue.push(ImGuiDatabaseEvent_CloseCreateTableWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::CloseCreateRelationalWindow()
{
	eventQueue.push(ImGuiDatabaseEvent_CloseCreateRelationalWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::CloseCreateConditionWindow()
{
	eventQueue.push(ImGuiDatabaseEvent_CloseCreateConditionWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::CloseRelationalWindow()
{
	eventQueue.push(ImGuiDatabaseEvent_CloseRelationalWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::CloseConditionWindow()
{
	eventQueue.push(ImGuiDatabaseEvent_CloseConditionWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::CloseDebugWindow()
{
	eventQueue.push(ImGuiDatabaseEvent_CloseDebugWindow);
}

void ImGuiDatabase::ImGuiDatabaseAppication::LoadDatabase(Mugenshuen::string_t dbName)
{
	database.CloseTable(false);
	database.CloseDatabsae(false);
	database.LoadDatabase(dbName);
}

void ImGuiDatabase::ImGuiDatabaseAppication::LoadTable(Mugenshuen::string_t tblName)
{
	table = database.LoadTable(tblName);
	dstRecordWindowEdit.reserve(table->NumColumn());
	dstTableWindowRecord.reserve(table->NumColumn());
	dstTableWindowColumnInitial.reserve(table->NumColumn());
	dstTableWindowColumnType.reserve(table->NumColumn());
	dstRelationalWindowRecord.reserve(table->NumColumn());
	
}

void ImGuiDatabase::ImGuiDatabaseAppication::SaveDatabase()
{
	if (CurrentDatabase() != "")
		database.SaveTable();
}

void ImGuiDatabase::ImGuiDatabaseAppication::SaveTable()
{
	if (table)
		database.SaveTable(table);
}

void ImGuiDatabase::ImGuiDatabaseAppication::CloseDatabase(bool save)
{
	if (CurrentDatabase() != "")
	{
		CloseTableWindow();
		CloseRelationalWindow();
		ExecuteEvent();
		database.CloseDatabsae(save);
	}
}

void ImGuiDatabase::ImGuiDatabaseAppication::CloseTable(bool save)
{
	database.CloseTable(save, table);
	table = nullptr;
}

void ImGuiDatabase::ImGuiDatabaseAppication::CreateDatabase(Mugenshuen::string_t dbName)
{
	database.CreateDatabase(dbName, MugenScript::DATABASE_PAGE_SIZE_1024);
}

void ImGuiDatabase::ImGuiDatabaseAppication::CreateTable(Mugenshuen::string_t tblName, int numColumn, Mugenshuen::string_t* names, MugenScript::DATABASE_VALUE_TYPE* types, MugenScript::MugenScriptDatabaseValue* initials)
{
	database.CreateTable(tblName.C_Str(), numColumn, names, types, initials);
}


void ImGuiDatabase::ImGuiDatabaseAppication::CreateRecord()
{
	if(table)
		recWindow.Create(this,table->NumColumn(), table->NamesColumn(), table->TypesColumn(), GetInitials(), dstRecordWindowEdit.data());
}

void ImGuiDatabase::ImGuiDatabaseAppication::CreateRecordData(int num, MugenScript::MugenScriptDatabaseValue*)
{
	if (table)
		table->Create(table->NumColumn(), dstRecordWindowEdit.data());
}

void ImGuiDatabase::ImGuiDatabaseAppication::CreateRelational()
{
	crelWindow.Create();
}

void ImGuiDatabase::ImGuiDatabaseAppication::CreateRelationalData(const Mugenshuen::string_t columnName, const Mugenshuen::string_t databaseName, const Mugenshuen::string_t tableName, const Mugenshuen::string_t relationalColumnName)
{
	if(table)
		table->CreateRelation(columnName, databaseName, tableName, relationalColumnName);
}

void ImGuiDatabase::ImGuiDatabaseAppication::DeleteDatabase(Mugenshuen::string_t dbName)
{
	if (IsLoadedDatabase(dbName))
		CloseDatabase(false);
	database.DeleteDatabase(dbName);
}

void ImGuiDatabase::ImGuiDatabaseAppication::DeleteTable(Mugenshuen::string_t tblName)
{
	database.DeleteTable(tblName);
}

void ImGuiDatabase::ImGuiDatabaseAppication::DeleteRecord(int id)
{
	if (table)
		table->Delete(id);
}

void ImGuiDatabase::ImGuiDatabaseAppication::DeleteRelational(const Mugenshuen::string_t columnName)
{
	if(table)
		table->DeleteRelation(columnName);
}

void ImGuiDatabase::ImGuiDatabaseAppication::EditRecord(int id)
{
	if (table)
	{
		table->Read(id, dstRecordWindowEdit.data());
		recWindow.Edit(this, id, table->NumColumn(), table->NamesColumn(), table->TypesColumn(), dstRecordWindowEdit.data());
	}
}

void ImGuiDatabase::ImGuiDatabaseAppication::EditRecordData(int id, MugenScript::MugenScriptDatabaseValue* value)
{
	if(table)
		table->Replace(id, value);
}

void ImGuiDatabase::ImGuiDatabaseAppication::EditRelational()
{
	crelWindow.Edit();
}

void ImGuiDatabase::ImGuiDatabaseAppication::EditRelationalData(const Mugenshuen::string_t columnName, const Mugenshuen::string_t databaseName, const Mugenshuen::string_t tableName, const Mugenshuen::string_t relationalColumnName)
{
	if (table)
		table->ReplaceRelation(columnName, databaseName, tableName, relationalColumnName);
}

void ImGuiDatabase::ImGuiDatabaseAppication::EditScript(const Mugenshuen::string_t name, const Mugenshuen::string_t& script, MugenScript::MugenScriptDatabaseValue* dst)
{
	scriptManager.BeginEditScript(name, script,dst);
	activeScriptManager = true;
}

void ImGuiDatabase::ImGuiDatabaseAppication::FocuseDatabaseWindow()
{
	ImGui::SetWindowFocus(dbWindow.Name().C_Str());
}

void ImGuiDatabase::ImGuiDatabaseAppication::FocuseTableWindow()
{
	ImGui::SetWindowFocus(tblWindow.Name().C_Str());
}

void ImGuiDatabase::ImGuiDatabaseAppication::FocuseRecordWindow()
{
	ImGui::SetWindowFocus(recWindow.Name().C_Str());
}

int ImGuiDatabase::ImGuiDatabaseAppication::GetNumDatabsaes()
{
	int count = 0;
	auto it = FileSystem::fm.file("resource");
	while (!it.last())
	{
		auto path = it.get();
		if (path.extension() == ".mdb")
			count++;
		it.next();
	}
	return count;
}

int ImGuiDatabase::ImGuiDatabaseAppication::GetNumTables()
{
	return database.NumTable();
}

int ImGuiDatabase::ImGuiDatabaseAppication::GetNumRecords()
{
	if (table)
		return table->NumRecord();
	else
		return 0;
}

int ImGuiDatabase::ImGuiDatabaseAppication::GetNumColumns()
{
	if (table)
		return table->NumColumn();
	else
		return 0;
}

int ImGuiDatabase::ImGuiDatabaseAppication::GetNumRecordsRelation(int columnID)
{
	if (table && table->GetRelation(columnID))
		return table->GetRelation(columnID)->relationTableIndex.Size();
}

int ImGuiDatabase::ImGuiDatabaseAppication::GetRecordID(int idx)
{
	if (table)
		return table->IDFromIndex(idx);
	return -1;
}

int ImGuiDatabase::ImGuiDatabaseAppication::GetRecordIDRelation(int idx, int columnID)
{
	int id = -1;
	int pos = -1;
	if (table && table->GetRelation(columnID))
		id = table->IDFromIndex(idx);
	return id;
}

int ImGuiDatabase::ImGuiDatabaseAppication::GetNumTables(Mugenshuen::string_t databaseName)
{

	MugenScript::MugenScriptDatabaseFile file;
	MugenScript::MugenScriptDatabaseReader reader;
	Mugenshuen::btree_t<MugenScript::MugenScriptDatabaseTableMapData> map;
	auto path = databaseName + ".mdb";
	file.open(path);
	reader.ReadDatabaseMap(map, file);
	file.close();

	return map.size();
}

Mugenshuen::string_t ImGuiDatabase::ImGuiDatabaseAppication::CurrentDatabase()
{
	return database.CurrentDatabase();
}

Mugenshuen::string_t ImGuiDatabase::ImGuiDatabaseAppication::GetNamesDatabase(int idx)
{
	int count = 0;
	auto it = FileSystem::fm.file("resource");

	while (!it.last() && it.get().extension() != ".mdb")
		it.next();

	Mugenshuen::string_t str;
	for (count = 0; count < idx && !it.last(); ++count)
	{
		it.next();
		while (!it.last() && it.get().extension() != ".mdb")
			it.next();
	}
	if (!it.last())
		return it.get().stem().string().c_str();
	else
		return "";
}

Mugenshuen::string_t ImGuiDatabase::ImGuiDatabaseAppication::GetNamesTable(int idx)
{
	return database.NameTable(idx);
}

void ImGuiDatabase::ImGuiDatabaseAppication::GetNamesTable(Mugenshuen::string_t database, Mugenshuen::string_t* dst)
{
	MugenScript::MugenScriptDatabaseFile file;
	MugenScript::MugenScriptDatabaseReader reader;
	Mugenshuen::btree_t<MugenScript::MugenScriptDatabaseTableMapData> map;
	auto path = database + ".mdb";
	file.open(path);
	reader.ReadDatabaseMap(map, file);
	file.close();

	for (int i = 0; i < map.size(); ++i)
		dst[i] = map[i].name;
}
int ImGuiDatabase::ImGuiDatabaseAppication::GetNamesColumn(MugenScript::DATABASE_VALUE_TYPE type, Mugenshuen::string_t database, Mugenshuen::string_t table, Mugenshuen::vector_t<Mugenshuen::string_t>& dst)
{
	MugenScript::MugenScriptDatabaseFile file;
	MugenScript::MugenScriptDatabaseColumn column(0, nullptr, nullptr, nullptr);
	MugenScript::MugenScriptDatabaseReader reader;
	MugenScript::MugenScriptDatabaseTableProp prop;
	Mugenshuen::btree_t<MugenScript::MugenScriptDatabaseTableMapData> map;
	int keyIdx = 0;
	MugenScript::MugenScriptDatabaseTableMapData data;
	auto path = database + ".mdb";
	data.name = table;
	file.open(path);
	reader.ReadDatabaseMap(map, file);

	auto n = map.find(keyIdx, data);
	if (!n)
	{
		file.close();
		return -1;
	}
	reader.ReadTableHandle(prop, n->keys[keyIdx].pos, file);
	reader.ReadTableColumn(n->keys[keyIdx].pos, prop, column, file);
	file.close();

	for (int i = 0; i < column.GetNumColumn(); ++i)
		if(column.GetType(i)==type)
			dst.push_back(column.GetName(i).value.string);
	column.Clear();

	return dst.size();
}
;

int ImGuiDatabase::ImGuiDatabaseAppication::GetNamesColumn(Mugenshuen::string_t database, Mugenshuen::string_t table, Mugenshuen::vector_t<Mugenshuen::string_t>& dst)
{
	MugenScript::MugenScriptDatabaseFile file;
	MugenScript::MugenScriptDatabaseColumn column(0, nullptr, nullptr, nullptr);
	MugenScript::MugenScriptDatabaseReader reader;
	MugenScript::MugenScriptDatabaseTableProp prop;
	Mugenshuen::btree_t<MugenScript::MugenScriptDatabaseTableMapData> map;
	int keyIdx = 0;
	MugenScript::MugenScriptDatabaseTableMapData data;
	auto path = database + ".mdb";
	data.name = table;
	file.open(path);
	reader.ReadDatabaseMap(map, file);
	
	auto n = map.find(keyIdx, data);
	if (!n)
	{
		file.close();
		return -1;
	}
	reader.ReadTableHandle(prop, n->keys[keyIdx].pos, file);
	reader.ReadTableColumn(n->keys[keyIdx].pos, prop, column, file);
	file.close();

	for (int i = 0; i < column.GetNumColumn(); ++i)
		dst.push_back(column.GetName(i).value.string);
	column.Clear();

	return dst.size();
}

MugenScript::MugenScriptDatabaseValue* ImGuiDatabase::ImGuiDatabaseAppication::GetRecord(int id)
{
	table->Read(id, dstTableWindowRecord.data());
	return dstTableWindowRecord.data();
}

MugenScript::MugenScriptDatabaseValue* ImGuiDatabase::ImGuiDatabaseAppication::GetRecordRelational(int id, int columnID)
{
	if (!table || id == -1)
		return nullptr;
	auto rel = table->GetRelation(columnID);
	if (!rel)
		return nullptr;
	if (rel->relationTableColumn.GetNumColumn() != dstRelationalWindowRecord[columnID].size())
		dstRelationalWindowRecord[columnID].reserve(rel->relationTableColumn.GetNumColumn());

	if (table->ReadRelational(id, columnID, dstRelationalWindowRecord[columnID].size(), dstRelationalWindowRecord[columnID].data()) != -1)
		return dstRelationalWindowRecord[columnID].data();
	else
		return nullptr;
}

void ImGuiDatabase::ImGuiDatabaseAppication::GetRelationalPath(const int columnID, Mugenshuen::string_t& database, Mugenshuen::string_t& table, Mugenshuen::string_t& column)
{
	if (!this->table || GetTypesColumn()[columnID] != MugenScript::DATABASE_VALUE_TYPE_EX_REL)
		return;

	auto rel = this->table->GetRelation(columnID);
	if (!rel)
		return;

	database = rel->databaseName;
	table = rel->tableName;
	column = rel->relationColumnName;
}


bool ImGuiDatabase::ImGuiDatabaseAppication::CheckCondition(int columnID,int recordID)
{
	if (table)
		return table->CheckCondition(columnID, recordID);
	return false;
}

const MugenScript::MugenScriptDatabaseValue* ImGuiDatabase::ImGuiDatabaseAppication::GetNamesColumn()
{
	if (table)
		return table->NamesColumn();
	else
		return nullptr;
}

const MugenScript::MugenScriptDatabaseValue* ImGuiDatabase::ImGuiDatabaseAppication::GetInitials()
{
	if(!table)
		return nullptr;

	for (int i = 0; i < table->NumColumn(); ++i)
	{
		dstTableWindowColumnInitial[i] = GetInitial(i);
	}
	return dstTableWindowColumnInitial.data();
}

const MugenScript::DATABASE_VALUE_TYPE* ImGuiDatabase::ImGuiDatabaseAppication::GetTypesColumn()
{
	if (table)
		return table->TypesColumn();
	else
		return nullptr;
}

const MugenScript::MugenScriptDatabaseValue ImGuiDatabase::ImGuiDatabaseAppication::GetInitial(int column)
{
	MugenScript::DATABASE_VALUE_TYPE type = GetTypesColumn()[column];
	switch (type)
	{
	case MugenScript::DATABASE_VALUE_TYPE_INT:
	case MugenScript::DATABASE_VALUE_TYPE_DBL:
	case MugenScript::DATABASE_VALUE_TYPE_STR:
		return table->InitialsColumn()[column];
		break;
	
	case MugenScript::DATABASE_VALUE_TYPE_EX_TRIGGER:
	case MugenScript::DATABASE_VALUE_TYPE_EX_REL:
		return GetInitialRelatioin(column);
		break;
	
	case MugenScript::DATABASE_VALUE_TYPE_EX_EVENT:
		return "";
		break;
	case MugenScript::DATABASE_VALUE_TYPE_EX_SCRIPT:
		return "";
		break;
	case MugenScript::DATABASE_VALUE_TYPE_INVALID:
		break;
	default:
		break;
	}
}

const MugenScript::MugenScriptDatabaseValue ImGuiDatabase::ImGuiDatabaseAppication::GetInitialCondition(int column)
{
	return MugenScript::MugenScriptDatabaseValue();
}

bool ImGuiDatabase::ImGuiDatabaseAppication::IsLoadedDatabase(Mugenshuen::string_t dbName)
{
	if (database.CurrentDatabase() == "")
		return false;

	return database.CurrentDatabase() == dbName;
}


bool ImGuiDatabase::ImGuiDatabaseAppication::IsTableExist(Mugenshuen::string_t tableName)
{
	return database.FindTable(tableName);
}

int ImGuiDatabase::ImGuiDatabaseAppication::GetNumColumnRelation(int columnID)
{
	if (table&&table->GetRelation(columnID))
		return table->GetRelation(columnID)->relationTableColumn.GetNumColumn();
	return 0;
}

MugenScript::DATABASE_VALUE_TYPE ImGuiDatabase::ImGuiDatabaseAppication::GetTypeRelation(int columnID)
{
	if (table)
	{
		auto rel = table->GetRelation(columnID);
		if (!rel)
			return MugenScript::DATABASE_VALUE_TYPE_INVALID;
		return rel->relationTableColumn.GetType(rel->relationColumn);
	}
	return MugenScript::DATABASE_VALUE_TYPE_INVALID;
}

MugenScript::DATABASE_VALUE_TYPE ImGuiDatabase::ImGuiDatabaseAppication::GetTypeRelation(Mugenshuen::string_t columnID)
{
	if (!table)
		return MugenScript::DATABASE_VALUE_TYPE_INVALID;

	for (int i = 0; i < table->NumColumn(); ++i)
		if (table->NameColumn(i) == columnID)
			return GetTypeRelation(i);
}

const MugenScript::MugenScriptDatabaseValue* ImGuiDatabase::ImGuiDatabaseAppication::GetColumnNameRelation(int columnID)
{
	if (table && table->GetRelation(columnID))
		return table->GetRelation(columnID)->relationTableColumn.GetNames();
	return nullptr;
}

void ImGuiDatabase::ImGuiDatabaseAppication::OnlyInput(Mugenshuen::string_t windowName)
{
	if (!onryInput.empty())
		onryInput.pop();
	onryInput.push(windowName.C_Str());
}

void ImGuiDatabase::ImGuiDatabaseAppication::ClearOnlyInput()
{
	while (!onryInput.empty())
		onryInput.pop();
}

void ImGuiDatabase::ImGuiDatabaseAppication::ExecuteEvent()
{
	while (!eventQueue.empty())
	{
		ImGuiDatabaseApplitionEvent eve = eventQueue.front();
		switch (eve)
		{
		case ImGuiDatabase::ImGuiDatabaseEvent_OpenDatabaseWindow:
			activeDatabaseWindow = true;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_CloseDatabaseWindow:
			activeDatabaseWindow = false;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_OpenTableWindow:
			activeTableWindow = true;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_OpenCreateDatabaseWindow:
			activeCreateDatabaseWindow = true;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_OpenCreateTableWindow:
			activeCreateTableWindow = true;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_OpenCreateRelationalWindow:
			activeCreateRelationWindow = true;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_OpenCreateConditionWindow:
			activeCreateConditionWindow = true;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_OpenConditionWindow:
			activeConditionWindow = true;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_OpenRecordWindow:
			activeRecordWindow = true;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_OpenRelationalWindow:
			activeRelationWindow = true;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_OpenDebugWindow:
			activeDebugWindow = true;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_CloseTableWindow:
			activeTableWindow = false;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_CloseRecordWindow:
			activeRecordWindow = false;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_CloseCreateDatabaseWindow:
			activeCreateDatabaseWindow = false;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_CloseCreateTableWindow:
			activeCreateTableWindow = false;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_CloseCreateRelationalWindow:
			activeCreateRelationWindow = false;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_CloseCreateConditionWindow:
			activeCreateConditionWindow = false;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_CloseConditionWindow:
			activeConditionWindow = false;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_CloseRelationalWindow:
			activeRelationWindow = false;
			break;
		case ImGuiDatabase::ImGuiDatabaseEvent_CloseDebugWindow:
			activeDebugWindow = false;
			break;
		default:
			break;
		}
		eventQueue.pop();
	}
}

ImGuiWindowFlags ImGuiDatabase::ImGuiDatabaseAppication::GetFlag(Mugenshuen::string_t windowName) const
{
	ImGuiWindowFlags ret = 0;
	if (!onryInput.empty() && windowName != onryInput.front().value.string)
		ret |= ImGuiWindowFlags_NoInputs;
	return ret;
}

MugenScript::MugenScriptDatabaseValue ImGuiDatabase::ImGuiDatabaseAppication::GetInitialRelatioin(int columnID)
{
	if (table)
	{
		auto rel = table->GetRelation(columnID);
		if (rel)
			return rel->relationTableColumn.GetInitial(rel->relationColumn);
	}
	return ((MugenScript::DATABASE_VALUE_TYPE)MugenScript::DATABASE_VALUE_TYPE_INVALID);
}

void ImGuiDatabase::ImGuiCreateDatabaseWindow::Draw(ImGuiDatabaseAppication* app)
{
	ImGui::SetWindowFocus();
	ImGui::Text("Database name");
	ImGui::SameLine();
	ImGui::InputText("##v", buff, 32);
	if (ImGui::Button("create") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))&&strlen(buff))
	{
		app->CreateDatabase(buff);
		app->CloseCreateDatabaseWindow();
	}
	if (ImGui::Button("cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)||(ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
	{
		app->CloseCreateDatabaseWindow();
	}
}

Mugenshuen::string_t ImGuiDatabase::ImGuiCreateDatabaseWindow::Name()
{
	return "Create Database";
}

ImGuiWindowFlags ImGuiDatabase::ImGuiCreateDatabaseWindow::Flags()
{
	return ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize;
}

void ImGuiDatabase::ImGuiCreateTableWindow::Draw(ImGuiDatabaseAppication* app)
{
	app->OnlyInput(Name());
	int select = -1;
	if (!name.value.string)
		name = "";
	nameInput.Input(app,-1,MugenScript::DATABASE_VALUE_TYPE_STR, name);
	ImGui::SameLine();
	ImGui::Text("table name");
	ImGui::InputInt("num column", &numColumn);
	if (numColumn < IMGUI_DATABASE_MIN_NUM_COLUMN)
		numColumn = IMGUI_DATABASE_MIN_NUM_COLUMN;
	if (IMGUI_DATABASE_MAX_NUM_COLUMN < numColumn)
		numColumn = IMGUI_DATABASE_MAX_NUM_COLUMN;
	if (numColumn != names.size())
	{
		names.resize(numColumn);
		types.resize(numColumn);
		initials.resize(numColumn);
		initialInput.resize(numColumn);
		comboString.resize(MugenScript::DATABASE_VALUE_TYPE_EX_NUM_TYPE);
		comboSelect.resize(MugenScript::DATABASE_VALUE_TYPE_EX_NUM_TYPE);
		for (int i = 0; i < numColumn; ++i)
			names[i].reserve(32);
		
		comboString[MugenScript::DATABASE_VALUE_TYPE_INT] = "int";
		comboString[MugenScript::DATABASE_VALUE_TYPE_DBL] = "double";
		comboString[MugenScript::DATABASE_VALUE_TYPE_STR] = "string";
		comboString[MugenScript::DATABASE_VALUE_TYPE_EX_REL] = "relation";
		comboString[MugenScript::DATABASE_VALUE_TYPE_EX_TRIGGER] = "trigger";
		comboString[MugenScript::DATABASE_VALUE_TYPE_EX_EVENT] = "event";
		comboString[MugenScript::DATABASE_VALUE_TYPE_EX_SCRIPT] = "script";
	}
	ImGui::NewLine();
	ImGui::Text("define column data");
	if (ImGui::BeginTable("create table", numColumn+1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
	{

		for (int i = 0; i < numColumn + 1; ++i)
			if(i==0)
				ImGui::TableSetupColumn("##v");
			else
				ImGui::TableSetupColumn(Mugenshuen::string_t(i).C_Str());
		ImGui::TableHeadersRow();
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::PushID("name_head");
		ImGui::Text("column name");
		ImGui::PopID();
		for (int column = 0; column < numColumn; ++column)
		{
			Mugenshuen::string_t idStr = "columnName";
			idStr += column;

			ImGui::TableSetColumnIndex(column + 1);
			ImGui::PushID(idStr.C_Str());
			ImGui::InputText("##v", names[column].Data(), 32);
			ImGui::PopID();
		}

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::PushID("type_head");
		ImGui::Text("value type");
		ImGui::PopID();
		for (int column = 0; column < numColumn; ++column)
		{
			Mugenshuen::string_t idStr = "columnType";
			idStr += column;

			ImGui::TableSetColumnIndex(column + 1);
			ImGui::PushID(idStr.C_Str());
			if (
				ImGui::Combo("##v", (int*)&types[column], comboString.data(), comboString.size())||
				ImGui::IsItemFocused()
				)
			{
				select = column;
			}
			ImGui::PopID();
		}

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::PushID("initial_head");
		ImGui::Text("initial value");
		ImGui::PopID();
		for (int column = 0; column < numColumn; ++column)
		{
			Mugenshuen::string_t idStr = "columnInitial";
			idStr += column;

			ImGui::TableSetColumnIndex(column + 1);
			ImGui::PushID(idStr.C_Str());
			if (types[column] < MugenScript::DATABASE_VALUE_TYPE_NUM_TYPE)
				initialInput[column].Input(app,-1,types[column], initials[column]);
			ImGui::PopID();
		}

		ImGui::EndTable();
	}

	if (select != -1 && ImGui::IsKeyPressed(ImGuiKey_Enter, false))
	{
		ImGui::OpenPopup("column_type", ImGuiPopupFlags_AnyPopup);
		selectColumn = select;
	}

	if (ImGui::BeginPopup("column_type"))
	{
		for (int i = 0; i < MugenScript::DATABASE_VALUE_TYPE_EX_NUM_TYPE; ++i)
			if (comboString[i]&&ImGui::MenuItem(comboString[i]) || ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false))
			{
				types[selectColumn] = i;
				initials[selectColumn] = MugenScript::MugenScriptDatabaseValue((MugenScript::DATABASE_VALUE_TYPE)i);
				ImGui::CloseCurrentPopup();
			}
		ImGui::EndPopup();
	}
	else
	{
		if (!ImGui::IsWindowFocused())
			ImGui::SetWindowFocus();
	}

	if (ImGui::Button("create")||(ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false)))
	{
		if (numColumn&&CheckNamesValid()&&!app->IsTableExist(name.value.string))
		{
			app->CreateTable(name.value.string, numColumn, names.data(), types.data(), initials.data());
			Close();
			app->CloseCreateTableWindow();
			app->ClearOnlyInput();
		}
	}
	if (ImGui::Button("cancel")||(ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false))||ImGui::IsKeyPressed(ImGuiKey_Escape,false))
	{
		Close();
		app->CloseCreateTableWindow();
		app->ClearOnlyInput();
	}


}

Mugenshuen::string_t ImGuiDatabase::ImGuiCreateTableWindow::Name()
{
	return "create_table_window";
}

ImGuiWindowFlags ImGuiDatabase::ImGuiCreateTableWindow::Flags()
{
	return ImGuiWindowFlags_NoDocking;
}

void ImGuiDatabase::ImGuiCreateTableWindow::Close()
{
	numColumn = 0;
	selectColumn = -1;
	name = "";
	names.clear();
	types.clear();
	initials.clear();
	comboString.clear();
	comboSelect.clear();
	initialInput.clear();
}

bool ImGuiDatabase::ImGuiCreateTableWindow::CheckNamesValid()
{
	for (int i = 0; i < numColumn; ++i)
		if (names[i] == "")
			return false;
	return true;
}

void ImGuiDatabase::ImGuiCreateRelationalWindow::Create()
{
	state = RELATIONAL_WINDOW_MODE_CREATE;
}

void ImGuiDatabase::ImGuiCreateRelationalWindow::Edit()
{
	state = RELATIONAL_WINDOW_MODE_EDIT;
}

void ImGuiDatabase::ImGuiCreateRelationalWindow::Draw(ImGuiDatabaseAppication* app)
{
	bool isOpenPopup = false;
	Mugenshuen::string_t selectedDB;
	{

		Mugenshuen::vector_t<Mugenshuen::string_t> dbNames;
		Mugenshuen::vector_t<const char*> pdbNames;
		int numDB = app->GetNumDatabsaes();
		dbNames.reserve(numDB);
		pdbNames.reserve(numDB);
		for (int i = 0; i < numDB; ++i)
		{
			dbNames.push_back(app->GetNamesDatabase(i));
			pdbNames.push_back(dbNames.back().C_Str());
			if (comboSelect[0] == i)
				selectedDB = dbNames.back();
		}

		ImGui::Text("Database");
		ImGui::Combo("##v_databases", &comboSelect[0], pdbNames.data(), numDB);
		if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))
			ImGui::OpenPopup("select_database");


		if (ImGui::BeginPopup("select_database"))
		{
			isOpenPopup = true;
			for (int i = 0; i < numDB; ++i)
			{
				if (ImGui::MenuItem(pdbNames[i]) || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
				{
					comboSelect[0] = i;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}
	}
	Mugenshuen::string_t selectedTable;
	{
		int numTable = app->GetNumTables(selectedDB);
		Mugenshuen::vector_t<Mugenshuen::string_t> tableNames;
		Mugenshuen::vector_t<const char*> pTableNames;
		tableNames.resize(numTable);
		pTableNames.resize(numTable);
		app->GetNamesTable(selectedDB, tableNames.data());
		for (int i = 0; i < numTable; ++i)
		{
			pTableNames[i] = tableNames[i].C_Str();
			if (comboSelect[1] == i)
				selectedTable = tableNames[i];
		}

		ImGui::Text("Table");
		ImGui::Combo("##v_tables", &comboSelect[1], pTableNames.data(), numTable);
		if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))
			ImGui::OpenPopup("select_table");

		if (ImGui::BeginPopup("select_table"))
		{
			isOpenPopup = true;
			for (int i = 0; i < numTable; ++i)
			{
				if (ImGui::MenuItem(pTableNames[i]) || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
				{
					comboSelect[1] = i;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}
	}
	Mugenshuen::string_t selectedColumn;
	{
		Mugenshuen::vector_t<Mugenshuen::string_t> columnNames;
		Mugenshuen::vector_t<const char*> pColumnNames;

		int numColumn = 0;
		if (state == RELATIONAL_WINDOW_MODE_CREATE)
			numColumn = app->GetNamesColumn(selectedDB, selectedTable, columnNames);
		else if (state == RELATIONAL_WINDOW_MODE_EDIT)
			numColumn = app->GetNamesColumn(app->GetTypeRelation(columnName), selectedDB, selectedTable, columnNames);

		for (int i = 0; i < numColumn; ++i)
		{
			pColumnNames.push_back(columnNames[i].C_Str());
			if (comboSelect[2] == i)
				selectedColumn = columnNames[i];
		}

		ImGui::Text("Column");
		ImGui::Combo("##v_columns", &comboSelect[2], pColumnNames.data(), numColumn);
		if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))
			ImGui::OpenPopup("select_column");

		if (ImGui::BeginPopup("select_column"))
		{
			isOpenPopup = true;
			for (int i = 0; i < numColumn; ++i)
			{
				if (ImGui::MenuItem(pColumnNames[i]) || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
				{
					comboSelect[2] = i;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}
	}
	Mugenshuen::string_t buttonName;
	if (state == RELATIONAL_WINDOW_MODE_CREATE)
		buttonName = "create";
	else if (state == RELATIONAL_WINDOW_MODE_EDIT) 
		buttonName = "edit";
	if (ImGui::Button(buttonName.C_Str()) || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false)))
	{
		if (selectedDB != "" && selectedTable != "" && selectedColumn != "")
		{
			app->CreateRelationalData(columnName, selectedDB, selectedTable, selectedColumn);
			app->CloseCreateRelationalWindow();
		}
	}
	if (ImGui::Button("cancel") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false)))
	{
		columnName = "";
		for (int i = 0; i < 3; ++i)
			comboSelect[i] = 0;
		app->CloseCreateRelationalWindow();
	}

	if (!isOpenPopup)
		ImGui::SetWindowFocus();
}


Mugenshuen::string_t ImGuiDatabase::ImGuiCreateRelationalWindow::Name()
{
	return "create_relation";
}

ImGuiWindowFlags ImGuiDatabase::ImGuiCreateRelationalWindow::Flags()
{
	return ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize;
}

void ImGuiDatabase::ImGuiCreateRelationalWindow::SetColumnName(Mugenshuen::string_t columnName)
{
	this->columnName = columnName;
}

void ImGuiDatabase::ImGuiRelationalWindow::Draw(ImGuiDatabaseAppication* app)
{
	if (app->GetNumColumns()< columnID)
	{
		ImGui::Text("Invalid column value");
		return;
	}
	if (!app->GetTypesColumn())
	{
		ImGui::Text("Table is not Loaded");
		return;
	}
	if (app->GetTypesColumn()[columnID] != MugenScript::DATABASE_VALUE_TYPE_EX_REL)
	{
		ImGui::Text("Invalid column type");
		return;
	}

	int numRecord = app->GetNumRecords();
	table.Set(app->GetNumColumnRelation(columnID), app->GetColumnNameRelation(columnID));
	table.Begin("relation", ImGuiTableFlags_Borders);

	table.BeginColumn();
	for (int i = 0; i < app->GetNumColumnRelation(columnID); ++i)
	{
		ImGui::TableNextColumn();
		ImGui::Text(app->GetColumnNameRelation(columnID)[i].value.string);

	}
	table.EndColumn();

	for (int i = 0; i < numRecord; ++i)
	{
		auto id = app->GetRecordIDRelation(i, columnID);
		auto rec = app->GetRecordRelational(id, columnID);
		if (rec)
		{
			table.BeginRecord(rec);
			table.EndRecord();
		}
		else
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("NaN");
		}
	}
	table.End();
}

Mugenshuen::string_t ImGuiDatabase::ImGuiRelationalWindow::Name()
{
	if (path == "")
		return "invalid path of relatioinal";
	else
		return path.C_Str();
}

ImGuiWindowFlags ImGuiDatabase::ImGuiRelationalWindow::Flags()
{
	return ImGuiWindowFlags_None;
}

void ImGuiDatabase::ImGuiRelationalWindow::SetColumnID(int columnID)
{
	this->columnID = columnID;
}

void ImGuiDatabase::ImGuiRelationalWindow::SetRelationalPath(Mugenshuen::string_t database, Mugenshuen::string_t table, Mugenshuen::string_t column)
{
	if (database == "" || table == "" || column == "")
		path = "";
	else
		path = database + "/" + table + "/" + column;
}

void ImGuiDatabase::ImGuiCreateConditionWindow::Draw(ImGuiDatabaseAppication* app)
{
	app->OnlyInput(Name());
	
	bool isOpenPopup = false;
	Mugenshuen::string_t selectedDB;
	{

		Mugenshuen::vector_t<Mugenshuen::string_t> dbNames;
		Mugenshuen::vector_t<const char*> pdbNames;
		int numDB = app->GetNumDatabsaes();
		dbNames.reserve(numDB);
		pdbNames.reserve(numDB);
		for (int i = 0; i < numDB; ++i)
		{
			dbNames.push_back(app->GetNamesDatabase(i));
			pdbNames.push_back(dbNames.back().C_Str());
			if (comboSelect[0] == i)
				selectedDB = dbNames.back();
		}

		ImGui::Text("Database");
		ImGui::Combo("##v_databases", &comboSelect[0], pdbNames.data(), numDB);
		if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))
			ImGui::OpenPopup("select_database");


		if (ImGui::BeginPopup("select_database"))
		{
			isOpenPopup = true;
			for (int i = 0; i < numDB; ++i)
			{
				if (ImGui::MenuItem(pdbNames[i]) || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
				{
					comboSelect[0] = i;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}
	}
	Mugenshuen::string_t selectedTable;
	{
		int numTable = app->GetNumTables(selectedDB);
		Mugenshuen::vector_t<Mugenshuen::string_t> tableNames;
		Mugenshuen::vector_t<const char*> pTableNames;
		tableNames.resize(numTable);
		pTableNames.resize(numTable);
		app->GetNamesTable(selectedDB, tableNames.data());
		for (int i = 0; i < numTable; ++i)
		{
			pTableNames[i] = tableNames[i].C_Str();
			if (comboSelect[1] == i)
				selectedTable = tableNames[i];
		}

		ImGui::Text("Table");
		ImGui::Combo("##v_tables", &comboSelect[1], pTableNames.data(), numTable);
		if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))
			ImGui::OpenPopup("select_table");

		if (ImGui::BeginPopup("select_table"))
		{
			isOpenPopup = true;
			for (int i = 0; i < numTable; ++i)
			{
				if (ImGui::MenuItem(pTableNames[i]) || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
				{
					comboSelect[1] = i;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}
	}
	Mugenshuen::string_t selectedColumn;
	{
		Mugenshuen::vector_t<Mugenshuen::string_t> columnNames;
		Mugenshuen::vector_t<const char*> pColumnNames;

		int numColumn = 0;
			numColumn = app->GetNamesColumn(selectedDB, selectedTable, columnNames);
		
		for (int i = 0; i < numColumn; ++i)
		{
			pColumnNames.push_back(columnNames[i].C_Str());
			if (comboSelect[2] == i)
				selectedColumn = columnNames[i];
		}

		ImGui::Text("Column");
		ImGui::Combo("##v_columns", &comboSelect[2], pColumnNames.data(), numColumn);
		if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))
			ImGui::OpenPopup("select_column");

		if (ImGui::BeginPopup("select_column"))
		{
			isOpenPopup = true;
			for (int i = 0; i < numColumn; ++i)
			{
				if (ImGui::MenuItem(pColumnNames[i]) || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
				{
					comboSelect[2] = i;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}
	}
	if ((ImGui::Button("set")||ImGui::IsItemFocused()&&ImGui::IsKeyPressed(ImGuiKey_Enter,false)) && columnID != relationID)
	{
		app->CloseCreateConditionWindow();
		app->ClearOnlyInput();
	}
	if (ImGui::Button("cancel") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
	{
		app->CloseCreateConditionWindow();
		app->ClearOnlyInput();
	}
}

Mugenshuen::string_t ImGuiDatabase::ImGuiCreateConditionWindow::Name()
{
	return "select reference column";
}

ImGuiWindowFlags ImGuiDatabase::ImGuiCreateConditionWindow::Flags()
{
	return ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize;
}

void ImGuiDatabase::ImGuiCreateConditionWindow::SetColumnID(int columnID)
{
	this->columnID = columnID;
}

void ImGuiDatabase::ImGuiConditionWindow::Draw(ImGuiDatabaseAppication* app)
{
	table.Set(1, &app->GetNamesColumn()[columnID]);
	auto numRecord = app->GetNumRecords();
	table.Begin("condition_result", ImGuiTableFlags_Borders);
	table.BeginColumn();
	table.EndColumn();
	for (int i = 0; i < numRecord; ++i)
	{
		MugenScript::MugenScriptDatabaseValue condResult(app->CheckCondition(columnID,app->GetRecordID(i)));
		table.BeginRecord(&condResult);
		table.EndRecord();
	}

	table.End();
}

Mugenshuen::string_t ImGuiDatabase::ImGuiConditionWindow::Name()
{
	return "result check condition";
}

ImGuiWindowFlags ImGuiDatabase::ImGuiConditionWindow::Flags()
{
	return ImGuiWindowFlags();
}

void ImGuiDatabase::ImGuiConditionWindow::SetColumnID(int id)
{
	columnID = id;
}

void ImGuiDatabase::ImGuiDebugWindow::Draw(ImGuiDatabaseAppication* app)
{
	if (!processQueue.empty() && !scriptQueue.empty())
	{
		auto path = processQueue.front();
		auto script = scriptQueue.front();
		processQueue.pop();
		scriptQueue.pop();
		int buffer = MultiByteToWideChar(CP_UTF8, 0, path.C_Str(), -1, nullptr, 0);
		auto wcBuff = new wchar_t[buffer];
		MultiByteToWideChar(CP_UTF8, 0, path.C_Str(), -1, wcBuff, buffer);
		dxLibDebugger.SetProperty(&debugger);
		debugger.StartProcess(wcBuff);
		delete[] wcBuff;

		StartDebugger(path, script);
	}
	debugger.UpdateDebuggerProcess();
	ImGui::Text("com counter : %d", debugger.GetCommunication());
	ImGui::Text("message counter : %d", debugger.GetMessageCounter());
	ImGui::Text("pc : %d", dxLibDebugger.GetPC());
	ImGui::Text("program size : %d", dxLibDebugger.GetProgSize());
	if (debugger.IsFinishedProcess())
	{
		app->CloseDebugWindow();
		debugger.EndProcess();
	}
}

Mugenshuen::string_t ImGuiDatabase::ImGuiDebugWindow::Name()
{
	return "debug_window";
}

ImGuiWindowFlags ImGuiDatabase::ImGuiDebugWindow::Flags()
{
	return ImGuiWindowFlags();
}

void ImGuiDatabase::ImGuiDebugWindow::PushProcessRequest(const Mugenshuen::string_t path)
{
	processQueue.push(path);
}

void ImGuiDatabase::ImGuiDebugWindow::PushLoadScript(const Mugenshuen::string_t script)
{
	scriptQueue.push(script);
}

void ImGuiDatabase::ImGuiDebugWindow::StartDebugger(const Mugenshuen::string_t processPath, const Mugenshuen::string_t script)
{
	auto dir = FileSystem::Parent(processPath.C_Str());
	auto path = dir.string() + "/debug_script.txt";
	auto dbPath = dir.string() + "/resource";
	file = FileSystem::fm.open(path.c_str(), FileSystem::FILE_MODE_BINARY);
	file.write((void*)script.C_Str(), script.Lengh());
	FileSystem::fm.close(file);
	FileSystem::RemoveAll(dbPath);
	FileSystem::Copy("resource", dbPath);

	dxLibDebugger.LoadScript(path.c_str());
}

void ImGuiDatabase::ImGuiScriptManager::Update()
{
	if (!file)
		return;
	FILETIME dummy;
	FILETIME dummy2;
	FILETIME writeTime;
	GetFileTime(file, &dummy, &dummy2, &writeTime);
	if (lastWriteTime.dwHighDateTime != writeTime.dwHighDateTime || lastWriteTime.dwLowDateTime != writeTime.dwLowDateTime)
	{
		Mugenshuen::string_t readBuf;
		readBuf.resize(FileSystem::FileSize(editScriptPath.C_Str()) + 1);
		auto it = FileSystem::fm.open(editScriptPath.C_Str(),FileSystem::FILE_MODE_BINARY);
		it.readall(readBuf.Data());
		FileSystem::fm.close(it);
		*value = readBuf.C_Str();
		lastWriteTime = writeTime;
	}
}

void ImGuiDatabase::ImGuiScriptManager::BeginEditScript(const Mugenshuen::string_t name, const Mugenshuen::string_t script, MugenScript::MugenScriptDatabaseValue* dst)
{
	auto path = name + ".mss";
	Mugenshuen::string_t cmd = "code ";
	FileSystem::CreateNewFile(path.C_Str());

	int buffer = MultiByteToWideChar(CP_UTF8, 0, path.C_Str(), -1, nullptr, 0);
	auto wcBuff = new wchar_t[buffer];
	MultiByteToWideChar(CP_UTF8, 0, path.C_Str(), -1, wcBuff, buffer);
	file = CreateFile(wcBuff, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	delete[] wcBuff;
	auto it = FileSystem::fm.open(path.C_Str(), FileSystem::FILE_MODE_BINARY);
	it.write((void*)script.C_Str(), script.Lengh()-1);
	FileSystem::fm.close(it);
	system((cmd + path).C_Str());
	editScriptPath = path;
	this->value = dst;
}


void ImGuiDatabase::ImGuiScriptManager::EndEditScript()
{
	CloseHandle(file);
	file = NULL;
}
