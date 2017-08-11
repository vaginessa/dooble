/*
** Copyright (c) 2008 - present, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from Dooble without specific prior written permission.
**
** DOOBLE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** DOOBLE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "dooble.h"
#include "dooble_cookies_window.h"
#include "dooble_cryptography.h"
#include "dooble_settings.h"

dooble_cookies_window::dooble_cookies_window(QWidget *parent):
  QMainWindow(parent)
{
  m_ui.setupUi(this);
  m_ui.tree->sortItems(0, Qt::AscendingOrder);
}

void dooble_cookies_window::show(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::
			      setting("dooble_cookies_window_geometry").
			      toByteArray()));

  QMainWindow::show();
}

void dooble_cookies_window::showNormal(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::
			      setting("dooble_cookies_window_geometry").
			      toByteArray()));

  QMainWindow::showNormal();
}

void dooble_cookies_window::slot_cookie_added(const QNetworkCookie &cookie,
					      bool is_favorite)
{
  if(!dooble::s_cryptography)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if(!m_top_level_items.contains(cookie.domain()))
    {
      QTreeWidgetItem *item = new QTreeWidgetItem
	(m_ui.tree, QStringList() << cookie.domain());

      if(is_favorite)
	item->setCheckState(0, Qt::Checked);
      else
	item->setCheckState(0, Qt::Unchecked);

      item->setFlags
	(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
      m_top_level_items[cookie.domain()] = item;
      m_ui.tree->addTopLevelItem(item);
    }
  else
    {
      QHash<QByteArray, QTreeWidgetItem *> hash
	(m_child_items.value(cookie.domain()));

      if(!hash.contains(cookie.name()))
	{
	  QTreeWidgetItem *item = new QTreeWidgetItem
	    (m_top_level_items[cookie.domain()],
	     QStringList() << "" << cookie.name());

	  hash[cookie.name()] = item;
	  item->setData(1, Qt::UserRole, cookie.toRawForm());
	  item->setFlags(Qt::ItemIsEnabled |
			 Qt::ItemIsSelectable |
			 Qt::ItemIsUserCheckable);
	  m_child_items[cookie.domain()] = hash;
	  m_top_level_items[cookie.domain()]->addChild(item);
	}
    }

  m_ui.tree->sortItems
    (m_ui.tree->sortColumn(), m_ui.tree->header()->sortIndicatorOrder());
  m_ui.tree->resizeColumnToContents(0);
  QApplication::restoreOverrideCursor();
}