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

#include <QHeaderView>
#include <QKeyEvent>
#include <QStandardItemModel>

#include "dooble.h"
#include "dooble_address_widget_completer.h"
#include "dooble_address_widget_completer_popup.h"
#include "dooble_favicons.h"
#include "dooble_history.h"
#include "dooble_page.h"

dooble_address_widget_completer::dooble_address_widget_completer
(QWidget *parent):QCompleter(parent)
{
  m_edit_timer.setInterval(100);
  m_edit_timer.setSingleShot(true);
  m_model = new QStandardItemModel(this);
  m_model->setSortRole(Qt::UserRole);
  m_popup = new dooble_address_widget_completer_popup(parent);
  m_popup->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  m_popup->horizontalHeader()->setVisible(false);
  m_popup->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_popup->setSelectionMode(QAbstractItemView::SingleSelection);
  m_popup->setShowGrid(false);
  m_popup->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  m_popup->verticalHeader()->setVisible(false);
  connect(&m_edit_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_edit_timer_timeout(void)));
  connect(m_popup,
	  SIGNAL(clicked(const QModelIndex &)),
	  this,
	  SLOT(slot_clicked(const QModelIndex &)));
  connect(qobject_cast<QLineEdit *> (parent),
	  SIGNAL(textEdited(const QString &)),
	  &m_edit_timer,
	  SLOT(start(void)));
  setCaseSensitivity(Qt::CaseInsensitive);
  setCompletionMode(QCompleter::UnfilteredPopupCompletion);
  setModel(m_model);
  setModelSorting(QCompleter::UnsortedModel);
  setPopup(m_popup);
}

int dooble_address_widget_completer::levenshtein_distance
(const QString &str1, const QString &str2) const
{
  if(str1.isEmpty())
    return str2.length();
  else if(str2.isEmpty())
    return str1.length();

  QChar str1_c = 0;
  QChar str2_c = 0;
  QVector<QVector<int> > matrix(str1.length() + 1,
				QVector<int> (str2.length() + 1));
  int cost = 0;

  for(int i = 0; i <= str1.length(); i++)
    matrix[i][0] = i;

  for(int i = 0; i <= str2.length(); i++)
    matrix[0][i] = i;

  for(int i = 1; i <= str1.length(); i++)
    {
      str1_c = str1.at(i - 1);

      for(int j = 1; j <= str2.length(); j++)
	{
	  str2_c = str2.at(j - 1);

	  if(str1_c == str2_c)
	    cost = 0;
	  else
	    cost = 1;

	  matrix[i][j] = qMin(qMin(matrix[i - 1][j] + 1,
				   matrix[i][j - 1] + 1),
			      matrix[i - 1][j - 1] + cost);
	}
    }

  return matrix[str1.length()][str2.length()];
}

void dooble_address_widget_completer::complete
(const QList<QWebEngineHistoryItem> &list)
{
  m_model->clear();
  m_model->setRowCount(list.size());

  for(int i = 0; i < m_model->rowCount(); i++)
    m_model->setItem
      (i, new QStandardItem(dooble_favicons::icon(list.at(i).url()),
			    list.at(i).url().toString()));

  if(m_model->rowCount())
    {
      m_popup->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      m_popup->setMaximumHeight
	(qMin(static_cast<int> (dooble_page::MAXIMUM_HISTORY_ITEMS),
	      m_model->rowCount()) * m_popup->rowHeight(0));
      m_popup->setMinimumHeight
	(qMin(static_cast<int> (dooble_page::MAXIMUM_HISTORY_ITEMS),
	      m_model->rowCount()) * m_popup->rowHeight(0));
      QCompleter::complete();
    }
}

void dooble_address_widget_completer::complete(const QString &text)
{
  m_model->clear();

  QList<QPair<QIcon, QString> > urls(dooble::s_history->urls());

  if(text.trimmed().isEmpty())
    {
      m_model->setRowCount(urls.size());

      for(int i = 0; i < m_model->rowCount(); i++)
	m_model->setItem
	  (i, new QStandardItem(urls.at(i).first, urls.at(i).second));
    }
  else
    {
      QList<QStandardItem *> list;
      QMultiMap<int, QStandardItem *> map;
      QString c(text.toLower().trimmed());

      for(int i = 0; i < urls.size(); i++)
	if(urls.at(i).second.toLower().contains(c))
	  map.insert(levenshtein_distance(c, urls.at(i).second.toLower()),
		     new QStandardItem(urls.at(i).first, urls.at(i).second));

      list = map.values();
      m_model->setRowCount(list.size());

      for(int i = 0; i < m_model->rowCount(); i++)
	m_model->setItem(i, list.at(i));
    }

  if(m_model->rowCount() > 0)
    {
      if(m_model->rowCount() > dooble_page::MAXIMUM_HISTORY_ITEMS)
	m_popup->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
      else
	m_popup->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

      m_popup->setMaximumHeight
	(qMin(static_cast<int> (dooble_page::MAXIMUM_HISTORY_ITEMS),
	      m_model->rowCount()) * m_popup->rowHeight(0));
      m_popup->setMinimumHeight
	(qMin(static_cast<int> (dooble_page::MAXIMUM_HISTORY_ITEMS),
	      m_model->rowCount()) * m_popup->rowHeight(0));
      QCompleter::complete();
    }
  else
    popup()->setVisible(false);
}

void dooble_address_widget_completer::slot_clicked(const QModelIndex &index)
{
  qobject_cast<QLineEdit *> (widget())->setText(index.data().toString());
  QApplication::postEvent
    (widget(), new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier));
}

void dooble_address_widget_completer::slot_edit_timer_timeout(void)
{
  QString text(qobject_cast<QLineEdit *> (widget())->text().trimmed());

  if(text.isEmpty())
    {
      m_model->clear();
      popup()->setVisible(false);
    }
  else
    complete(text);
}