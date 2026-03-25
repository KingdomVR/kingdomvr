/*=====================================================================
AvatarSettingsDialog.cpp
------------------------
Copyright Glare Technologies Limited 2022 -
=====================================================================*/
#include "AvatarSettingsDialog.h"


#include "AddObjectDialog.h"
#include "ModelLoading.h"
#include "AnimationManager.h"
#include "../shared/ResourceManager.h"
#include "../indigo/TextureServer.h"
#include "graphics/SRGBUtils.h"
#include "../dll/include/IndigoMesh.h"
#include "../dll/include/IndigoException.h"
#include "../dll/IndigoStringUtils.h"
#include "../utils/FileUtils.h"
#include "../utils/Exception.h"
#include "../utils/PlatformUtils.h"
#include "../utils/StringUtils.h"
#include "../utils/ConPrint.h"
#include "../utils/FileChecksum.h"
#include "../utils/FileInStream.h"
#include "../utils/TaskManager.h"
#include "../qt/QtUtils.h"
#include "../qt/SignalBlocker.h"
#include <QtCore/QStringList>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QErrorMessage>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtGui/QIcon>
#include <QtGui/QPixmap>
#include <algorithm>


AvatarSettingsDialog::AvatarSettingsDialog(const std::string& base_dir_path_, QSettings* settings_, Reference<ResourceManager> resource_manager_, AnimationManager* anim_manager_,
	const std::string& logged_in_username_, DownloadingResourceQueue* download_queue_, const std::vector<AvatarLibraryEntry>& server_avatar_library_)
:	base_dir_path(base_dir_path_),
	settings(settings_),
	resource_manager(resource_manager_),
	done_initial_load(false),
	pre_ob_to_world_matrix(Matrix4f::identity()),
	anim_manager(anim_manager_),
	logged_in_username(logged_in_username_),
	download_queue(download_queue_),
	server_avatar_library(server_avatar_library_),
	use_server_avatar_selection(false)
{
	setupUi(this);

	texture_server = new TextureServer(/*use_canonical_path_keys=*/false);

	this->usernameLabel->hide();
	this->usernameLineEdit->hide();

	this->avatarPreviewGLWidget->init(base_dir_path, settings_, texture_server);

	// Load main window geometry and state
	this->restoreGeometry(settings->value("AvatarSettingsDialog/geometry").toByteArray());

	//this->usernameLineEdit->setText(settings->value("username").toString());

	{
		SignalBlocker b(this->avatarSelectWidget);
		this->avatarSelectWidget->setType(FileSelectWidget::Type_File);
		this->avatarSelectWidget->setFilename(settings->value("avatarPath").toString());
	}

	this->serverAvatarListWidget->setViewMode(QListWidget::IconMode);
	this->serverAvatarListWidget->setIconSize(QSize(96, 96));
	this->serverAvatarListWidget->setResizeMode(QListWidget::Adjust);
	this->serverAvatarListWidget->setMovement(QListView::Static);

	const bool use_server_avatar = settings->value("AvatarSettingsDialog/useServerAvatar", false).toBool();
	{
		SignalBlocker b(this->avatarTabWidget);
		this->avatarTabWidget->setCurrentIndex(use_server_avatar ? 0 : 1);
	}

	loadCustomAvatarPaths();
	refreshServerAvatarList();

	if(use_server_avatar)
	{
		const URLString preferred_url = toURLString(QtUtils::toStdString(settings->value("AvatarSettingsDialog/serverAvatarURL").toString()));
		for(int i=0; i<this->serverAvatarListWidget->count(); ++i)
		{
			QListWidgetItem* item = this->serverAvatarListWidget->item(i);
			const int entry_i = item->data(Qt::UserRole).toInt();
			if(entry_i >= 0 && entry_i < (int)server_avatar_library.size() && server_avatar_library[entry_i].model_URL == preferred_url)
			{
				this->serverAvatarListWidget->setCurrentItem(item);
				break;
			}
		}

		if(this->serverAvatarListWidget->currentItem())
			serverAvatarSelected(this->serverAvatarListWidget->currentItem());
	}

	connect(this->avatarSelectWidget, SIGNAL(filenameChanged(QString&)), this, SLOT(avatarFilenameChanged(QString&)));
	connect(this->avatarTabWidget, SIGNAL(currentChanged(int)), this, SLOT(avatarTabChanged(int)));
	connect(this->serverAvatarListWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(serverAvatarSelected(QListWidgetItem*)));
	connect(this->customAvatarListWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(customAvatarSelected(QListWidgetItem*)));
	connect(this->buttonBox, SIGNAL(accepted()), this, SLOT(accepted()));
	connect(this, SIGNAL(finished(int)), this, SLOT(dialogFinished()));

	connect(this->animationComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(animationComboBoxIndexChanged(int)));

	startTimer(10);
	avatarTabChanged(this->avatarTabWidget->currentIndex());
}


AvatarSettingsDialog::~AvatarSettingsDialog()
{
	settings->setValue("AvatarSettingsDialog/geometry", saveGeometry());
}


void AvatarSettingsDialog::shutdownGL()
{
	// Make sure we have set the widget gl context to current as we destroy OpenGL stuff.
	this->avatarPreviewGLWidget->makeCurrent();

	preview_gl_ob = NULL;
	avatarPreviewGLWidget->shutdown();
}


//std::string AvatarSettingsDialog::getAvatarName()
//{
//	return QtUtils::toStdString(usernameLineEdit->text());
//}


// Called when user presses ESC key, or clicks OK or cancel button.
void AvatarSettingsDialog::dialogFinished()
{
	//this->settings->setValue("username", this->usernameLineEdit->text());

	shutdownGL();
}


void AvatarSettingsDialog::accepted()
{
	if(this->avatarTabWidget->currentIndex() == 0)
	{
		this->use_server_avatar_selection = true;
		this->settings->setValue("AvatarSettingsDialog/useServerAvatar", true);
		this->settings->setValue("AvatarSettingsDialog/serverAvatarURL", QtUtils::toQString(toStdString(this->selected_server_avatar_URL)));
		this->settings->setValue("avatarPath", QString());
	}
	else
	{
		this->use_server_avatar_selection = false;
		this->settings->setValue("AvatarSettingsDialog/useServerAvatar", false);
		this->settings->setValue("AvatarSettingsDialog/serverAvatarURL", QString());
		this->settings->setValue("avatarPath", this->avatarSelectWidget->filename());

		const std::string path = QtUtils::toStdString(this->avatarSelectWidget->filename());
		if(!path.empty())
		{
			custom_avatar_paths.erase(std::remove(custom_avatar_paths.begin(), custom_avatar_paths.end(), path), custom_avatar_paths.end());
			custom_avatar_paths.insert(custom_avatar_paths.begin(), path);
			if(custom_avatar_paths.size() > 10)
				custom_avatar_paths.resize(10);
			saveCustomAvatarPaths();
		}
	}
}


void AvatarSettingsDialog::avatarFilenameChanged(QString& filename)
{
	if(this->avatarTabWidget->currentIndex() != 1)
		return;

	const std::string path = QtUtils::toIndString(filename);
	const bool changed = this->result_path != path;
	this->result_path = path;
	this->use_server_avatar_selection = false;
	this->selected_server_avatar_URL.clear();
	
	// conPrint("AvatarSettingsDialog::avatarFilenameChanged: filename = " + path);

	if(changed)
	{
		loadModelIntoPreview(path, /*show_error_dialogs=*/true);
	}
}


void AvatarSettingsDialog::avatarTabChanged(int index)
{
	const bool server_tab = (index == 0);

	this->avatarPreviewGLWidget->setVisible(!server_tab);
	this->animationComboBox->setVisible(!server_tab);
	this->label->setVisible(!server_tab);

	if(server_tab)
	{
		this->use_server_avatar_selection = true;
		if(this->serverAvatarListWidget->currentItem())
			serverAvatarSelected(this->serverAvatarListWidget->currentItem());
		else
			refreshServerAvatarThumbnail();
	}
}


void AvatarSettingsDialog::serverAvatarSelected(QListWidgetItem* item)
{
	if(item == NULL)
		return;

	const int entry_i = item->data(Qt::UserRole).toInt();
	if(entry_i < 0 || entry_i >= (int)server_avatar_library.size())
		return;

	this->use_server_avatar_selection = true;
	this->selected_server_avatar_URL = server_avatar_library[entry_i].model_URL;
	this->result_path.clear();
	this->loaded_mesh = NULL;
	this->loaded_materials.clear();
	this->pre_ob_to_world_matrix = Matrix4f::identity();

	if(download_queue)
		download_queue->enqueueOrUpdateItem(server_avatar_library[entry_i].model_URL, Vec4f(0, 0, 0, 1), 1.f);

	refreshServerAvatarThumbnail();
}


void AvatarSettingsDialog::customAvatarSelected(QListWidgetItem* item)
{
	if(item == NULL)
		return;

	this->avatarSelectWidget->setFilename(item->text());
}


void AvatarSettingsDialog::animationComboBoxIndexChanged(int index)
{
	if(preview_gl_ob.isNull())
		return;

	const std::string anim_name = QtUtils::toStdString(this->animationComboBox->itemText(index));
	preview_gl_ob->current_anim_i = myMax(0, preview_gl_ob->mesh_data->animation_data.getAnimationIndex(anim_name));
}


static const char* anim_names[] = {
	"Walking",
	"Idle",
	"Walking Backward",
	"Running",
	"Running Backward",
	"Floating",
	"Flying",
	"Left Turn",
	"Right Turn",
};


void AvatarSettingsDialog::loadModelIntoPreview(const std::string& local_path, bool show_error_dialogs)
{
	const std::string use_local_path = local_path.empty() ? 
		(base_dir_path + "/data/resources/xbot_glb_3242545562312850498.bmesh") :
		local_path;

	this->avatarPreviewGLWidget->makeCurrent();

	this->pre_ob_to_world_matrix = Matrix4f::identity();

	// Try and load model
	try
	{
		if(preview_gl_ob.nonNull())
			avatarPreviewGLWidget->opengl_engine->removeObject(preview_gl_ob); // Remove previous object from engine.

		ModelLoading::MakeGLObjectResults results;
		ModelLoading::makeGLObjectForModelFile(*avatarPreviewGLWidget->opengl_engine, *avatarPreviewGLWidget->opengl_engine->vert_buf_allocator, /*allocator=*/nullptr, use_local_path, /*do_opengl_stuff=*/true,
			results
		);

		this->preview_gl_ob = results.gl_ob;
		this->loaded_mesh = results.batched_mesh;
		this->loaded_materials = results.materials;

		if(local_path.empty()) // If we used xbot_glb_3242545562312850498.bmesh we need to rotate it upright
		{
			this->preview_gl_ob->ob_to_world_matrix = Matrix4f::rotationAroundXAxis(Maths::pi_2<float>());

			this->preview_gl_ob->materials[0].albedo_linear_rgb = toLinearSRGB(Avatar::defaultMat0Col());
			this->preview_gl_ob->materials[0].metallic_frac = Avatar::default_mat0_metallic_frac;
			this->preview_gl_ob->materials[0].roughness = Avatar::default_mat0_roughness;
			this->preview_gl_ob->materials[0].albedo_texture = NULL;
			this->preview_gl_ob->materials[0].tex_path.clear();

			this->preview_gl_ob->materials[1].albedo_linear_rgb = toLinearSRGB(Avatar::defaultMat1Col());
			this->preview_gl_ob->materials[1].metallic_frac = Avatar::default_mat1_metallic_frac;
			this->preview_gl_ob->materials[1].albedo_texture = NULL;
			this->preview_gl_ob->materials[1].tex_path.clear();


			loaded_materials.resize(2);
			loaded_materials[0] = new WorldMaterial();
			loaded_materials[0]->colour_rgb = Avatar::defaultMat0Col();
			loaded_materials[0]->metallic_fraction.val = Avatar::default_mat0_metallic_frac;
			loaded_materials[0]->roughness.val = Avatar::default_mat0_roughness;

			loaded_materials[1] = new WorldMaterial();
			loaded_materials[1]->colour_rgb = Avatar::defaultMat1Col();
			loaded_materials[1]->metallic_fraction.val = Avatar::default_mat1_metallic_frac;
		}

		/*Vec4f original_left_eye_pos = preview_gl_ob->mesh_data->animation_data.getNodePositionModelSpace("LeftEye");
		if(original_left_eye_pos == Vec4f(0,0,0,1))
		{
			assert(0);
			original_left_eye_pos = Vec4f(0,1.67,0,1);
		}*/

		Vec4f original_toe_pos = preview_gl_ob->mesh_data->animation_data.getNodePositionModelSpace("LeftToe_End", /*use_retarget_adjustment=*/false);

		// TEMP: Load animation data for ready-player-me type avatars
		//float eye_height_adjustment = 0;
		float foot_bottom_height = original_toe_pos[1] - 0.0362269469; // Should be ~= 0
		//printVar(foot_bottom_height);
		if(true)
		{
			// Append the first animation (Idle) and build the retargetting data.
			preview_gl_ob->mesh_data->animation_data.loadAndRetargetAnim(*anim_manager->getAnimation("Idle.subanim", *resource_manager));
		
			// Append all the other animations
			for(size_t i=0; i<staticArrayNumElems(anim_names); ++i)
				preview_gl_ob->mesh_data->animation_data.appendAnimationData(*anim_manager->getAnimation(URLString(anim_names[i]) + ".subanim", *resource_manager));


			// If we loaded the extracted_avatar_anim bone data, then the avatar will be floating off the ground for female leg lengths, so move down.
			//eye_height_adjustment = -1.67 + original_left_eye_pos[1];

			Vec4f new_toe_pos = preview_gl_ob->mesh_data->animation_data.getNodePositionModelSpace("LeftToe_End", /*use_retarget_adjustment=*/true);
			conPrint("new_toe_pos: " + new_toe_pos.toStringNSigFigs(4));

			foot_bottom_height = new_toe_pos[1] - 0.03; // Height of foot bottom for avatar with retargetted animation, off ground.

			conPrint("foot_bottom_height: " + doubleToStringNSigFigs(foot_bottom_height, 4));
		}

		// Populate current animation combobox with anim names
		animationComboBox->clear();
		for(size_t i=0; i<preview_gl_ob->mesh_data->animation_data.animations.size(); ++i)
			animationComboBox->addItem(QtUtils::toQString(preview_gl_ob->mesh_data->animation_data.animations[i]->name));
		animationComboBox->setMaxVisibleItems(50);

		// Select Idle animation initially
		preview_gl_ob->current_anim_i = myMax(0, preview_gl_ob->mesh_data->animation_data.getAnimationIndex("Idle"));
		SignalBlocker::setCurrentIndex(animationComboBox, preview_gl_ob->current_anim_i);

		// Construct transformation to bring ready-player-me avatars to z-up and standing on the ground.
		// We want to translate the avatar down from 1.67 metres in the sky (which is the default substrata eye height), to the ground
		this->pre_ob_to_world_matrix = Matrix4f::translationMatrix(0, 0, -1.67 - foot_bottom_height) * preview_gl_ob->ob_to_world_matrix;

		preview_gl_ob->ob_to_world_matrix = Matrix4f::translationMatrix(0, 0, -foot_bottom_height) * preview_gl_ob->ob_to_world_matrix;

		// Try and load textures
		AddObjectDialog::tryLoadTexturesForPreviewOb(preview_gl_ob, this->loaded_materials, avatarPreviewGLWidget->opengl_engine.ptr(), *texture_server, this);

		avatarPreviewGLWidget->opengl_engine->addObject(preview_gl_ob);
	}
	catch(Indigo::IndigoException& e)
	{
		this->loaded_mesh = NULL;

		if(show_error_dialogs)
			QtUtils::showErrorMessageDialog(QtUtils::toQString(e.what()), this);
	}
	catch(glare::Exception& e)
	{
		this->loaded_mesh = NULL;

		if(show_error_dialogs)
			QtUtils::showErrorMessageDialog(QtUtils::toQString(e.what()), this);
	}
}


// Will be called when the user clicks the 'X' button.
void AvatarSettingsDialog::closeEvent(QCloseEvent* event)
{
	shutdownGL();
}


void AvatarSettingsDialog::timerEvent(QTimerEvent* event)
{
	avatarPreviewGLWidget->makeCurrent();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	avatarPreviewGLWidget->update();
#else
	avatarPreviewGLWidget->updateGL();
#endif

	// Once the OpenGL widget has initialised, we can add the model.
	if(avatarPreviewGLWidget->opengl_engine.nonNull() && avatarPreviewGLWidget->opengl_engine->initSucceeded() && !done_initial_load)
	{
		const QString path = settings->value("avatarPath").toString();
		this->result_path = QtUtils::toStdString(path);
		if(this->avatarTabWidget->currentIndex() == 1)
			loadModelIntoPreview(QtUtils::toStdString(path), /*show_error_dialogs=*/false);
		done_initial_load = true;
	}

	static int tick_count = 0;
	tick_count++;
	if((tick_count % 50) == 0)
	{
		refreshServerAvatarList();
		refreshServerAvatarThumbnail();
	}
}


std::string AvatarSettingsDialog::customAvatarSettingsKey() const
{
	std::string key_username = logged_in_username.empty() ? "anonymous" : logged_in_username;
	for(size_t i=0; i<key_username.size(); ++i)
		if(!::isAlphaNumeric(key_username[i]))
			key_username[i] = '_';

	return "AvatarSettingsDialog/customAvatarPaths/" + key_username;
}


void AvatarSettingsDialog::loadCustomAvatarPaths()
{
	custom_avatar_paths.clear();
	this->customAvatarListWidget->clear();

	const QStringList paths = settings->value(QtUtils::toQString(customAvatarSettingsKey())).toStringList();
	for(int i=0; i<paths.size(); ++i)
	{
		const std::string path = QtUtils::toStdString(paths[i]);
		if(FileUtils::fileExists(path))
		{
			custom_avatar_paths.push_back(path);
			this->customAvatarListWidget->addItem(paths[i]);
		}
	}
}


void AvatarSettingsDialog::saveCustomAvatarPaths()
{
	QStringList paths;
	for(size_t i=0; i<custom_avatar_paths.size(); ++i)
		paths.push_back(QtUtils::toQString(custom_avatar_paths[i]));

	settings->setValue(QtUtils::toQString(customAvatarSettingsKey()), paths);

	this->customAvatarListWidget->clear();
	for(size_t i=0; i<custom_avatar_paths.size(); ++i)
		this->customAvatarListWidget->addItem(QtUtils::toQString(custom_avatar_paths[i]));
}


void AvatarSettingsDialog::refreshServerAvatarList()
{
	const URLString selected_url = this->selected_server_avatar_URL;

	this->serverAvatarListWidget->clear();
	for(size_t i=0; i<server_avatar_library.size(); ++i)
	{
		QListWidgetItem* item = new QListWidgetItem(QtUtils::toQString(server_avatar_library[i].display_name));
		item->setData(Qt::UserRole, (int)i);

		if(!server_avatar_library[i].thumbnail_URL.empty())
		{
			if(resource_manager->isFileForURLPresent(server_avatar_library[i].thumbnail_URL))
			{
				const std::string thumb_path = resource_manager->pathForURL(server_avatar_library[i].thumbnail_URL);
				if(FileUtils::fileExists(thumb_path))
					item->setIcon(QIcon(QtUtils::toQString(thumb_path)));
			}
			else if(download_queue)
				download_queue->enqueueOrUpdateItem(server_avatar_library[i].thumbnail_URL, Vec4f(0, 0, 0, 1), 1.f);
		}

		this->serverAvatarListWidget->addItem(item);
	}

	for(int i=0; i<this->serverAvatarListWidget->count(); ++i)
	{
		QListWidgetItem* item = this->serverAvatarListWidget->item(i);
		const int entry_i = item->data(Qt::UserRole).toInt();
		if(entry_i >= 0 && entry_i < (int)server_avatar_library.size() && server_avatar_library[entry_i].model_URL == selected_url)
		{
			this->serverAvatarListWidget->setCurrentItem(item);
			break;
		}
	}
}


void AvatarSettingsDialog::refreshServerAvatarThumbnail()
{
	if(this->serverAvatarListWidget->currentItem() == NULL)
	{
		this->serverAvatarThumbLabel->setText("Select a server avatar.");
		this->serverAvatarThumbLabel->setPixmap(QPixmap());
		return;
	}

	const int entry_i = this->serverAvatarListWidget->currentItem()->data(Qt::UserRole).toInt();
	if(entry_i < 0 || entry_i >= (int)server_avatar_library.size())
		return;

	const URLString thumb_URL = server_avatar_library[entry_i].thumbnail_URL;
	if(thumb_URL.empty())
	{
		this->serverAvatarThumbLabel->setText("No thumbnail available for this avatar.");
		this->serverAvatarThumbLabel->setPixmap(QPixmap());
		return;
	}

	if(resource_manager->isFileForURLPresent(thumb_URL))
	{
		const std::string thumb_path = resource_manager->pathForURL(thumb_URL);
		QPixmap pixmap(QtUtils::toQString(thumb_path));
		if(!pixmap.isNull())
		{
			this->serverAvatarThumbLabel->setPixmap(pixmap.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation));
			this->serverAvatarThumbLabel->setText("");
			return;
		}
	}

	if(download_queue)
		download_queue->enqueueOrUpdateItem(thumb_URL, Vec4f(0, 0, 0, 1), 1.f);

	this->serverAvatarThumbLabel->setText("Downloading thumbnail...");
	this->serverAvatarThumbLabel->setPixmap(QPixmap());
}
