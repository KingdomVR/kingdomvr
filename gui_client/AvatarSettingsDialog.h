/*=====================================================================
AvatarSettingsDialog.h
----------------------
Copyright Glare Technologies Limited 2022 -
=====================================================================*/
#pragma once


#include "ui_AvatarSettingsDialog.h"
#include "../dll/include/IndigoMesh.h"
#include "DownloadingResourceQueue.h"
#include "../shared/WorldMaterial.h"
#include "../shared/WorldObject.h"
#include "../shared/URLString.h"
#include <utils/ThreadManager.h>
#include <graphics/BatchedMesh.h>
#include <QtCore/QString>
class QSettings;
class AnimationManager;
struct GLObject;
class QListWidgetItem;


/*=====================================================================
AvatarSettingsDialog
--------------------

=====================================================================*/
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4324) // Disable 'structure was padded due to __declspec(align())' warning.
#endif

class AvatarSettingsDialog : public QDialog, private Ui_AvatarSettingsDialog
{
	Q_OBJECT
public:
	struct AvatarLibraryEntry
	{
		std::string display_name;
		URLString model_URL;
		URLString thumbnail_URL;
	};

	AvatarSettingsDialog(const std::string& base_dir_path_, QSettings* settings, Reference<ResourceManager> resource_manager, AnimationManager* anim_manager,
		const std::string& logged_in_username_, DownloadingResourceQueue* download_queue_, const std::vector<AvatarLibraryEntry>& server_avatar_library_);
	~AvatarSettingsDialog();

	//std::string getAvatarName();
private slots:;
	void accepted();
	void dialogFinished();

	void avatarFilenameChanged(QString& filename);
	void avatarTabChanged(int index);
	void serverAvatarSelected(QListWidgetItem* item);
	void serverAvatarSearchChanged(const QString& text);

	void animationComboBoxIndexChanged(int index);
	
private:
	virtual void closeEvent(QCloseEvent *event);
	virtual void timerEvent(QTimerEvent* event);

	void loadModelIntoPreview(const std::string& local_path, bool show_error_dialogs);
	void refreshServerAvatarList();

	void shutdownGL();

	QSettings* settings;

	Reference<GLObject> preview_gl_ob;

	bool done_initial_load;
public:
	std::string result_path;

	BatchedMeshRef loaded_mesh; // May by NULL if a valid object was not loaded.

	std::vector<WorldMaterialRef> loaded_materials;

	std::string base_dir_path;

	Reference<ResourceManager> resource_manager;

	Matrix4f pre_ob_to_world_matrix;

	Reference<TextureServer> texture_server;
	AnimationManager* anim_manager;

	std::string logged_in_username;
	DownloadingResourceQueue* download_queue;
	std::vector<AvatarLibraryEntry> server_avatar_library;
	QString server_avatar_filter;

public:
	bool use_server_avatar_selection;
	URLString selected_server_avatar_URL;
};

#ifdef _WIN32
#pragma warning(pop)
#endif
